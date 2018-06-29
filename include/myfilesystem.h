/** @file
 * @brief std::filesystem is still not available on most of the systems I use, so
 * implement the required parts here.
 *
 * Currently only works on posix systems
 *
 * @author Mark Grimes (mark.grimes@rymapt.com)
 * @date 29/Jun/2018
 * @copyright Copyright Rymapt Ltd, released under the MIT licence (https://opensource.org/licenses/MIT)
 */
#pragma once
#include <string>
#include <system_error>

namespace myfilesystem
{
	std::string canonical( const char* path, std::error_code& error );
	std::string canonical( const char* path );

	inline std::string canonical( const std::string& path, std::error_code& error ) { return canonical(path.c_str(),error); }
	inline std::string canonical( const std::string& path ) { return canonical(path.c_str()); }
} // end of namespace myfilesystem
