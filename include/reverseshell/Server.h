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
#ifndef INCLUDEGUARD_reverseshell_Server_h
#define INCLUDEGUARD_reverseshell_Server_h

#include <memory>
#include <functional>
namespace reverseshell
{
	class Connection;
}


namespace reverseshell
{
	/** @brief The server that accepts connections from Clients and provides a command interface to their shells.
	 *
	 * @author Mark Grimes (mark.grimes@rymapt.com)
	 * @date 14/Nov/2017
	 * @copyright Copyright 2017 Rymapt Ltd. Released under the MIT licence (available from https://opensource.org/licenses/MIT).
	 */
	class Server
	{
	public:
		Server();
		Server( Server&& otherServer ) noexcept;
		~Server();

		/** @brief Listen on the given port. */
		void listen( size_t port );
		/** @brief Start the event loop, blocking until the server is stopped. */
		void run();

		/** @brief Stops the server listening and unblocks the caller of "listen()". Must be called from another thread. */
		void stop();

		void setCertificateChainFile( const std::string& filename );
		void setPrivateKeyFile( const std::string& filename );
		void setVerifyFile( const std::string& filename );

		void setNewConnectionCallback( std::function<void(reverseshell::Connection& connection)> connection );

		/** @brief The port being listened on. Useful if "0" was passed to listen() to test using OS allocated ports. */
		int port() const;
	private:
		/// Pimple idiom to hide the transport details
		std::unique_ptr<class ServerPrivateMembers> pImple_;
	};

} // end of namespace reverseshell

#endif
