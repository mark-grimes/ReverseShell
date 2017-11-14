#ifndef INCLUDEGUARD_reverseshell_Client_h
#define INCLUDEGUARD_reverseshell_Client_h

#include <memory>

namespace reverseshell
{
	/** @brief The client that connects out to a known server and offers the server a shell
	 *
	 * @author Mark Grimes (mark.grimes@rymapt.com)
	 * @date 14/Nov/2017
	 * @copyright Copyright 2017 Rymapt Ltd. Licence To be decided.
	 */
	class Client
	{
	public:
		Client();
		Client( Client&& otherClient ) noexcept;
		~Client();

		/** @brief Attempts to connect to the URI provided. Blocks until the connection has been terminated. */
		void connect( const std::string& URI );

		void setCertificateChainFile( const std::string& filename );
		void setPrivateKeyFile( const std::string& filename );
		void setVerifyFile( const std::string& filename );

	private:
		/// Pimple idiom to hide the transport details
		std::unique_ptr<class ClientPrivateMembers> pImple_;
	};

} // end of namespace reverseshell

#endif
