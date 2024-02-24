#include "http_message.hpp"

#include <sstream>
#include <string>

namespace couringserver {
std::string http_response::serialize() const
{
	std::stringstream raw_http_response;
	raw_http_response << version << ' ' << status << ' ' << status_text << "\r\n";
	for (const auto &[k, v] : header_list)
	{
		raw_http_response << k << ':' << v << "\r\n";
	}
	raw_http_response << "\r\n";
	return raw_http_response.str();
}
} // namespace couringserver
