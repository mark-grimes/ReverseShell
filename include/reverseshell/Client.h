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
#ifndef INCLUDEGUARD_reverseshell_Client_h
#define INCLUDEGUARD_reverseshell_Client_h

#include <memory>

namespace reverseshell
{
	/** @brief The client that connects out to a known server and offers the server a shell
	 *
	 * @author Mark Grimes (mark.grimes@rymapt.com)
	 * @date 14/Nov/2017
	 * @copyright Copyright 2017 Rymapt Ltd. Released under the MIT licence (available from https://opensource.org/licenses/MIT).
	 */
	class Client
	{
	public:
		Client();
		Client( Client&& otherClient ) noexcept;
		~Client();

		/** @brief Attempts to connect to the URI provided. Blocks until the connection has been terminated. */
		void connect( const std::string& URI );
		void disconnect();

		void setCertificateChainFile( const std::string& filename );
		void setPrivateKeyFile( const std::string& filename );
		void setVerifyFile( const std::string& filename );

	private:
		/// Pimple idiom to hide the transport details
		std::unique_ptr<class ClientPrivateMembers> pImple_;
	};

} // end of namespace reverseshell

#endif
