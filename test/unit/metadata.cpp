/*
 * metadata.cpp
 *
 *  Created on: Oct 27, 2019
 *      Author: ruben
 */

#include "mysql/metadata.hpp"
#include <gtest/gtest.h>
#include "mysql/impl/serialization.hpp"

using namespace testing;
using namespace mysql::detail;

namespace
{

TEST(FieldMetadata, IntPrimaryKey)
{
	column_definition_packet msg {
		string_lenenc("def"),
		string_lenenc("awesome"),
		string_lenenc("test_table"),
		string_lenenc("test_table"),
		string_lenenc("id"),
		string_lenenc("id"),
		mysql::collation::binary,
		int4(11),
		protocol_field_type::long_,
		int2(column_flags::pri_key | column_flags::auto_increment | column_flags::not_null),
		int1(0)
	};
	mysql::field_metadata meta (msg);

	EXPECT_EQ(meta.database(), "awesome");
	EXPECT_EQ(meta.table(), "test_table");
	EXPECT_EQ(meta.original_table(), "test_table");
	EXPECT_EQ(meta.field_name(), "id");
	EXPECT_EQ(meta.original_field_name(), "id");
	EXPECT_EQ(meta.column_length(), 11);
	EXPECT_EQ(meta.type(), mysql::field_type::int_);
	EXPECT_EQ(meta.protocol_type(), protocol_field_type::long_);
	EXPECT_EQ(meta.decimals(), 0);
    EXPECT_TRUE(meta.is_not_null());
    EXPECT_TRUE(meta.is_primary_key());
    EXPECT_FALSE(meta.is_unique_key());
    EXPECT_FALSE(meta.is_multiple_key());
    EXPECT_FALSE(meta.is_unsigned());
    EXPECT_FALSE(meta.is_zerofill());
    EXPECT_TRUE(meta.is_auto_increment());
    EXPECT_FALSE(meta.has_no_default_value());
    EXPECT_FALSE(meta.is_set_to_now_on_update());
}

TEST(FieldMetadata, VarcharWithAlias)
{
	column_definition_packet msg {
		string_lenenc("def"),
		string_lenenc("awesome"),
		string_lenenc("child"),
		string_lenenc("child_table"),
		string_lenenc("field_alias"),
		string_lenenc("field_varchar"),
		mysql::collation::utf8_general_ci,
		int4(765),
		protocol_field_type::var_string,
		int2(0),
		int1(0)
	};
	mysql::field_metadata meta (msg);

	EXPECT_EQ(meta.database(), "awesome");
	EXPECT_EQ(meta.table(), "child");
	EXPECT_EQ(meta.original_table(), "child_table");
	EXPECT_EQ(meta.field_name(), "field_alias");
	EXPECT_EQ(meta.original_field_name(), "field_varchar");
	EXPECT_EQ(meta.column_length(), 765);
	EXPECT_EQ(meta.protocol_type(), protocol_field_type::var_string);
	EXPECT_EQ(meta.type(), mysql::field_type::varchar);
	EXPECT_EQ(meta.decimals(), 0);
	EXPECT_FALSE(meta.is_not_null());
	EXPECT_FALSE(meta.is_primary_key());
	EXPECT_FALSE(meta.is_unique_key());
	EXPECT_FALSE(meta.is_multiple_key());
	EXPECT_FALSE(meta.is_unsigned());
	EXPECT_FALSE(meta.is_zerofill());
	EXPECT_FALSE(meta.is_auto_increment());
	EXPECT_FALSE(meta.has_no_default_value());
	EXPECT_FALSE(meta.is_set_to_now_on_update());
}

TEST(FieldMetadata, FloatField)
{
	column_definition_packet msg {
		string_lenenc("def"),
		string_lenenc("awesome"),
		string_lenenc("test_table"),
		string_lenenc("test_table"),
		string_lenenc("field_float"),
		string_lenenc("field_float"),
		mysql::collation::binary,
		int4(12),
		protocol_field_type::float_,
		int2(0),
		int1(31)
	};
	mysql::field_metadata meta (msg);

	EXPECT_EQ(meta.database(), "awesome");
	EXPECT_EQ(meta.table(), "test_table");
	EXPECT_EQ(meta.original_table(), "test_table");
	EXPECT_EQ(meta.field_name(), "field_float");
	EXPECT_EQ(meta.original_field_name(), "field_float");
	EXPECT_EQ(meta.column_length(), 12);
	EXPECT_EQ(meta.protocol_type(), protocol_field_type::float_);
	EXPECT_EQ(meta.type(), mysql::field_type::float_);
	EXPECT_EQ(meta.decimals(), 31);
    EXPECT_FALSE(meta.is_not_null());
    EXPECT_FALSE(meta.is_primary_key());
    EXPECT_FALSE(meta.is_unique_key());
    EXPECT_FALSE(meta.is_multiple_key());
    EXPECT_FALSE(meta.is_unsigned());
    EXPECT_FALSE(meta.is_zerofill());
    EXPECT_FALSE(meta.is_auto_increment());
    EXPECT_FALSE(meta.has_no_default_value());
    EXPECT_FALSE(meta.is_set_to_now_on_update());

}

} // anon namespace
