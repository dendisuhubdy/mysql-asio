/*
 * resultset.cpp
 *
 *  Created on: Feb 5, 2020
 *      Author: ruben
 */

#include "integration_test_common.hpp"
#include <boost/asio/use_future.hpp>

using namespace mysql::test;
using mysql::detail::make_error_code;
using mysql::test::meta_validator;
using mysql::test::validate_meta;
using mysql::field_metadata;
using mysql::field_type;
using mysql::error_code;
using mysql::error_info;
using mysql::tcp_resultset;
namespace net = boost::asio;

namespace
{

class resultset_generator
{
public:
	virtual ~resultset_generator() {}
	virtual const char* name() const = 0;
	virtual mysql::tcp_resultset generate(mysql::tcp_connection&, std::string_view) = 0;
};

struct ResultsetTestParam : named_param
{
	resultset_generator* gen;
	network_functions* net;

	ResultsetTestParam(resultset_generator* gen, network_functions* net):
		gen(gen), net(net) {}
};

std::ostream& operator<<(std::ostream& os, const ResultsetTestParam& v)
{
	return os << v.gen->name() << '.' << v.net->name();
}

struct ResultsetTest : public IntegTestAfterHandshake,
					   public testing::WithParamInterface<ResultsetTestParam>
{
	auto do_generate(std::string_view query) { return GetParam().gen->generate(conn, query); }
	auto do_fetch_one(tcp_resultset& r) { return GetParam().net->fetch_one(r); }
	auto do_fetch_many(tcp_resultset& r, std::size_t count) { return GetParam().net->fetch_many(r, count); }
	auto do_fetch_all(tcp_resultset& r) { return GetParam().net->fetch_all(r); }
};

// FetchOne
TEST_P(ResultsetTest, FetchOne_NoResults)
{
	auto result = do_generate("SELECT * FROM empty_table");
	EXPECT_TRUE(result.valid());
	EXPECT_FALSE(result.complete());
	EXPECT_EQ(result.fields().size(), 2);

	// Already in the end of the resultset, we receive the EOF
	auto row_result = do_fetch_one(result);
	row_result.validate_no_error();
	EXPECT_EQ(row_result.value, nullptr);
	validate_eof(result);

	// Fetching again just returns null
	row_result = do_fetch_one(result);
	row_result.validate_no_error();
	EXPECT_EQ(row_result.value, nullptr);
	validate_eof(result);
}

TEST_P(ResultsetTest, FetchOne_OneRow)
{
	auto result = do_generate("SELECT * FROM one_row_table");
	EXPECT_TRUE(result.valid());
	EXPECT_FALSE(result.complete());
	EXPECT_EQ(result.fields().size(), 2);

	// Fetch only row
	auto row_result = do_fetch_one(result);
	row_result.validate_no_error();
	ASSERT_NE(row_result.value, nullptr);
	validate_2fields_meta(result, "one_row_table");
	EXPECT_EQ(row_result.value->values(), makevalues(1, "f0"));
	EXPECT_FALSE(result.complete());

	// Fetch next: end of resultset
	row_result = do_fetch_one(result);
	row_result.validate_no_error();
	ASSERT_EQ(row_result.value, nullptr);
	validate_eof(result);
}

TEST_P(ResultsetTest, FetchOne_TwoRows)
{
	auto result = do_generate("SELECT * FROM two_rows_table");
	EXPECT_TRUE(result.valid());
	EXPECT_FALSE(result.complete());
	EXPECT_EQ(result.fields().size(), 2);

	// Fetch first row
	auto row_result = do_fetch_one(result);
	row_result.validate_no_error();
	ASSERT_NE(row_result.value, nullptr);
	validate_2fields_meta(result, "two_rows_table");
	EXPECT_EQ(row_result.value->values(), makevalues(1, "f0"));
	EXPECT_FALSE(result.complete());

	// Fetch next row
	row_result = do_fetch_one(result);
	row_result.validate_no_error();
	ASSERT_NE(row_result.value, nullptr);
	validate_2fields_meta(result, "two_rows_table");
	EXPECT_EQ(row_result.value->values(), makevalues(2, "f1"));
	EXPECT_FALSE(result.complete());

	// Fetch next: end of resultset
	row_result = do_fetch_one(result);
	row_result.validate_no_error();
	ASSERT_EQ(row_result.value, nullptr);
	validate_eof(result);
}

// There seems to be no real case where fetch can fail (other than net fails)

// FetchMany
TEST_P(ResultsetTest, FetchMany_NoResults)
{
	auto result = do_generate("SELECT * FROM empty_table");

	// Fetch many, but there are no results
	auto rows_result = do_fetch_many(result, 10);
	rows_result.validate_no_error();
	EXPECT_TRUE(rows_result.value.empty());
	EXPECT_TRUE(result.complete());
	validate_eof(result);

	// Fetch again, should return OK and empty
	rows_result = do_fetch_many(result, 10);
	rows_result.validate_no_error();
	EXPECT_TRUE(rows_result.value.empty());
	EXPECT_TRUE(result.complete());
	validate_eof(result);
}

TEST_P(ResultsetTest, FetchMany_MoreRowsThanCount)
{
	auto result = do_generate("SELECT * FROM three_rows_table");

	// Fetch 2, one remaining
	auto rows_result = do_fetch_many(result, 2);
	rows_result.validate_no_error();
	EXPECT_FALSE(result.complete());
	EXPECT_EQ(rows_result.value, (makerows(2, 1, "f0", 2, "f1")));

	// Fetch another two (completes the resultset)
	rows_result = do_fetch_many(result, 2);
	rows_result.validate_no_error();
	EXPECT_TRUE(result.complete());
	validate_eof(result);
	EXPECT_EQ(rows_result.value, (makerows(2, 3, "f2")));
}

TEST_P(ResultsetTest, FetchMany_LessRowsThanCount)
{
	auto result = do_generate("SELECT * FROM two_rows_table");

	// Fetch 3, resultset exhausted
	auto rows_result = do_fetch_many(result, 3);
	rows_result.validate_no_error();
	EXPECT_EQ(rows_result.value, (makerows(2, 1, "f0", 2, "f1")));
	validate_eof(result);
}

TEST_P(ResultsetTest, FetchMany_SameRowsAsCount)
{
	auto result = do_generate("SELECT * FROM two_rows_table");

	// Fetch 2, 0 remaining but resultset not exhausted
	auto rows_result = do_fetch_many(result, 2);
	rows_result.validate_no_error();
	EXPECT_FALSE(result.complete());
	EXPECT_EQ(rows_result.value, (makerows(2, 1, "f0", 2, "f1")));

	// Fetch again, exhausts the resultset
	rows_result = do_fetch_many(result, 2);
	rows_result.validate_no_error();
	EXPECT_EQ(rows_result.value.size(), 0);
	validate_eof(result);
}

TEST_P(ResultsetTest, FetchMany_CountEqualsOne)
{
	auto result = do_generate("SELECT * FROM one_row_table");

	// Fetch 1, 1 remaining
	auto rows_result = do_fetch_many(result, 1);
	rows_result.validate_no_error();
	EXPECT_FALSE(result.complete());
	EXPECT_EQ(rows_result.value, (makerows(2, 1, "f0")));
}

// FetchAll
TEST_P(ResultsetTest, FetchAll_NoResults)
{
	auto result = do_generate("SELECT * FROM empty_table");

	// Fetch many, but there are no results
	auto rows_result = do_fetch_all(result);
	rows_result.validate_no_error();
	EXPECT_TRUE(rows_result.value.empty());
	EXPECT_TRUE(result.complete());

	// Fetch again, should return OK and empty
	rows_result = do_fetch_all(result);
	rows_result.validate_no_error();
	EXPECT_TRUE(rows_result.value.empty());
	validate_eof(result);
}

TEST_P(ResultsetTest, FetchAll_OneRow)
{
	auto result = do_generate("SELECT * FROM one_row_table");

	auto rows_result = do_fetch_all(result);
	rows_result.validate_no_error();
	EXPECT_TRUE(result.complete());
	EXPECT_EQ(rows_result.value, (makerows(2, 1, "f0")));
}

TEST_P(ResultsetTest, FetchAll_SeveralRows)
{
	auto result = do_generate("SELECT * FROM two_rows_table");

	auto rows_result = do_fetch_all(result);
	rows_result.validate_no_error();
	validate_eof(result);
	EXPECT_TRUE(result.complete());
	EXPECT_EQ(rows_result.value, (makerows(2, 1, "f0", 2, "f1")));
}


// Instantiate the test suites
class text_resultset_generator : public resultset_generator
{
public:
	const char* name() const override { return "text"; }
	mysql::tcp_resultset generate(mysql::tcp_connection& conn, std::string_view query) override
	{
		return conn.query(query);
	}
};

class binary_resultset_generator : public resultset_generator
{
public:
	const char* name() const override { return "binary"; }
	mysql::tcp_resultset generate(mysql::tcp_connection& conn, std::string_view query) override
	{
		return conn.prepare_statement(query).execute(mysql::no_statement_params);
	}
};

text_resultset_generator text_obj;
binary_resultset_generator binary_obj;

resultset_generator* all_resultset_generators [] = {
	&text_obj,
	&binary_obj
};

std::vector<ResultsetTestParam> make_resultset_test_params()
{
	std::vector<ResultsetTestParam> res;
	for (auto* gen: all_resultset_generators)
	{
		for (auto* net: all_network_functions)
		{
			res.push_back(ResultsetTestParam{gen, net});
		}
	}
	return res;
}


INSTANTIATE_TEST_SUITE_P(Default, ResultsetTest, testing::ValuesIn(
	make_resultset_test_params()
), test_name_generator);

}

