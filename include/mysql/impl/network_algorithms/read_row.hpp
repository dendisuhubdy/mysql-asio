#ifndef INCLUDE_MYSQL_IMPL_NETWORK_ALGORITHMS_READ_TEXT_ROW_HPP_
#define INCLUDE_MYSQL_IMPL_NETWORK_ALGORITHMS_READ_TEXT_ROW_HPP_

#include "mysql/impl/network_algorithms/common.hpp"
#include "mysql/metadata.hpp"
#include <string_view>
#include <vector>

namespace mysql
{
namespace detail
{

enum class read_row_result
{
	error,
	row,
	eof
};

template <typename StreamType>
read_row_result read_row(
	deserialize_row_fn deserializer,
	channel<StreamType>& channel,
	const std::vector<field_metadata>& meta,
	bytestring& buffer,
	std::vector<value>& output_values,
	ok_packet& output_ok_packet,
	error_code& err,
	error_info& info
);

template <typename StreamType, typename CompletionToken>
BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, void(error_code, error_info, read_row_result))
async_read_row(
	deserialize_row_fn deserializer,
	channel<StreamType>& channel,
	const std::vector<field_metadata>& meta,
	bytestring& buffer,
	std::vector<value>& output_values,
	ok_packet& output_ok_packet,
	CompletionToken&& token
);


}
}

#include "mysql/impl/network_algorithms/read_row.ipp"



#endif /* INCLUDE_MYSQL_IMPL_NETWORK_ALGORITHMS_READ_TEXT_ROW_HPP_ */
