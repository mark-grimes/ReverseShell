/* MIT licence:
 * Copyright 2017 Rymapt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in the
 * Software without restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
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
