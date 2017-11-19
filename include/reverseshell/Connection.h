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
#ifndef INCLUDEGUARD_reverseshell_Connection_h
#define INCLUDEGUARD_reverseshell_Connection_h

#include <memory>
#include <functional>
namespace reverseshell
{
	class ServerPrivateMembers;
}

namespace reverseshell
{
	/** @brief Represents the server side connection from a single client.
	 *
	 * @author Mark Grimes (mark.grimes@rymapt.com)
	 * @date 18/Nov/2017
	 * @copyright Copyright 2017 Rymapt Ltd. Released under the MIT licence (available from https://opensource.org/licenses/MIT).
	 */
	class Connection
	{
	public:
		~Connection();

		void send( const char* message, size_t size );
		void send( const std::string& message );
		void setStdOutCallback( std::function<void(const char*, size_t)> callback );
		void setStdErrCallback( std::function<void(const char*, size_t)> callback );
	private:
		friend class reverseshell::ServerPrivateMembers;
		Connection( std::function<void(const char*, size_t)> sendFunction );
		const std::function<void(const char*, size_t)>& stdout();
		const std::function<void(const char*, size_t)>& stderr();
		/// Pimple idiom to hide the transport details
		std::shared_ptr<class ConnectionPrivateMembers> pImple_;
	};

} // end of namespace reverseshell

#endif
