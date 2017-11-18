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
#include "reverseshell/Server.h"

#include <list>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio.hpp>
#include "reverseshell/Client.h"

//
// Declaration of the pimple
//
namespace reverseshell
{
	class ServerPrivateMembers
	{
	public:
		typedef websocketpp::server<websocketpp::config::asio_tls> server_type;
		server_type server_;
		std::list<websocketpp::connection_hdl> currentConnections_;
		mutable std::mutex currentConnectionsMutex_;
		bool isListening_;
		mutable std::mutex isListeningMutex_;
		std::string certificateChainFileName_;
		std::string privateKeyFileName_;
		std::string verifyFileName_;
		std::string diffieHellmanParamsFileName_;

		ServerPrivateMembers();
		bool removeConnection( websocketpp::connection_hdl hdl );
		void on_http( websocketpp::connection_hdl hdl );
		void on_open( websocketpp::connection_hdl hdl );
		void on_close( websocketpp::connection_hdl hdl );
		void on_interrupt( websocketpp::connection_hdl hdl );
		void on_message( websocketpp::connection_hdl hdl, server_type::message_ptr );
		std::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init( websocketpp::connection_hdl hdl );
	};
}

reverseshell::Server::Server()
	: pImple_( new ServerPrivateMembers )
{
	// No operation. Most work done in the ServerPrivateMembers constructor.
}

reverseshell::Server::Server( Server&& otherServer ) noexcept
	: pImple_( std::move(otherServer.pImple_) )
{
	// No operation, everything done in initialiser list
}

reverseshell::Server::~Server()
{
	// No operation
}

void reverseshell::Server::listen( size_t port )
{
	{
		std::lock_guard<std::mutex> isListeningLock(pImple_->isListeningMutex_);
		pImple_->isListening_=true;
	}
	websocketpp::lib::error_code errorCode;
	pImple_->server_.listen( port, errorCode );
	if( errorCode )
	{
		std::lock_guard<std::mutex> isListeningLock(pImple_->isListeningMutex_);
		pImple_->isListening_=false;
		throw std::runtime_error( "reverseshell::Server listen error: "+errorCode.message() );
	}

	pImple_->server_.start_accept();
}

void reverseshell::Server::run()
{
	pImple_->server_.run();
}

void reverseshell::Server::stop()
{
	std::lock_guard<std::mutex> isListeningLock(pImple_->isListeningMutex_);
	if( pImple_->isListening_ )
	{
		// In case listen() was called but it is still in the process of activating
		while( !pImple_->server_.is_listening() ) std::this_thread::sleep_for( std::chrono::milliseconds(50) );
		pImple_->server_.stop_listening();
		pImple_->isListening_=false;
	}

	// First send the EOF signal (blank string) to the terminal on the other end to make the
	// shell close properly. Wait a little while, and if the connection is still open force
	// it to close.
	std::unique_lock<std::mutex> currentConnectionsLock( pImple_->currentConnectionsMutex_ );
	std::cout << "Stopping server with " << pImple_->currentConnections_.size() << " open connections" << std::endl;
	for( auto& handle : pImple_->currentConnections_ )
	{
		auto pConnection=pImple_->server_.get_con_from_hdl(handle);
		if( pConnection ) pConnection->send(""); // This is interpreted as EOF on the other end
	}
	// Give the connections a chance to close themselves
	for( size_t attempts=0; attempts<10 && !pImple_->currentConnections_.empty(); ++attempts )
	{
		currentConnectionsLock.unlock();
		std::this_thread::sleep_for( std::chrono::milliseconds(50) );
		currentConnectionsLock.lock();
	}
	// Force close any that are still open
	std::cout << "Force closing " << pImple_->currentConnections_.size() << " open connections" << std::endl;
	for( auto& handle : pImple_->currentConnections_ )
	{
		auto pConnection=pImple_->server_.get_con_from_hdl(handle);
		if( pConnection )
		{
			std::error_code errorCode;
			pConnection->close( websocketpp::close::status::normal, "Server shutdown", errorCode );

			if( errorCode ) std::cerr << "reverseshell::Server::stop() - " << errorCode.message() << std::endl;
		}
	}
	pImple_->currentConnections_.clear();
}

void reverseshell::Server::setCertificateChainFile( const std::string& filename )
{
	pImple_->certificateChainFileName_=filename;
}

void reverseshell::Server::setPrivateKeyFile( const std::string& filename )
{
	pImple_->privateKeyFileName_=filename;
}

void reverseshell::Server::setVerifyFile( const std::string& filename )
{
	pImple_->verifyFileName_=filename;
}

reverseshell::ServerPrivateMembers::ServerPrivateMembers()
	: isListening_(false)
{
	server_.set_access_channels(websocketpp::log::alevel::none);
	//server_.set_error_channels(websocketpp::log::elevel::all ^ websocketpp::log::elevel::info);
	server_.set_error_channels(websocketpp::log::elevel::all);
	//server_.set_error_channels(websocketpp::log::elevel::none);
	server_.set_tls_init_handler( std::bind( &ServerPrivateMembers::on_tls_init, this, std::placeholders::_1 ) );
	server_.set_http_handler( std::bind( &ServerPrivateMembers::on_http, this, std::placeholders::_1 ) );
	server_.set_open_handler( std::bind( &ServerPrivateMembers::on_open, this, std::placeholders::_1 ) );
	server_.set_close_handler( std::bind( &ServerPrivateMembers::on_close, this, std::placeholders::_1 ) );
	server_.set_interrupt_handler( std::bind( &ServerPrivateMembers::on_interrupt, this, std::placeholders::_1 ) );
	server_.set_message_handler( std::bind( &ServerPrivateMembers::on_message, this, std::placeholders::_1, std::placeholders::_2 ) );
	server_.init_asio();
}

bool reverseshell::ServerPrivateMembers::removeConnection( websocketpp::connection_hdl hdl )
{
	auto pConnection=server_.get_con_from_hdl(hdl);
	std::lock_guard<std::mutex> currentConnectionsLock( currentConnectionsMutex_ );
	auto findResult=std::find_if( currentConnections_.begin(), currentConnections_.end(),
			[pConnection,this](const websocketpp::connection_hdl& storedHandle)
			{
				return pConnection==server_.get_con_from_hdl(storedHandle);
			} );
	if( findResult!=currentConnections_.end() )
	{
		currentConnections_.erase( findResult );
		return true;
	}
	else return false;
}

void reverseshell::ServerPrivateMembers::on_http( websocketpp::connection_hdl hdl )
{
	std::cout << "HTTP connection requested" << std::endl;
}

void reverseshell::ServerPrivateMembers::on_open( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has opened on the server" << std::endl;
	auto pConnection=server_.get_con_from_hdl(hdl);
	// Send an arbitrary command while testing
	pConnection->send("ls\n");
	std::lock_guard<std::mutex> myMutex( currentConnectionsMutex_ );
	currentConnections_.emplace_back( hdl );
}

void reverseshell::ServerPrivateMembers::on_close( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has closed on the server" << std::endl;
	if( !removeConnection(hdl) ) std::cout << "Couldn't find connection to remove" << std::endl;
}

void reverseshell::ServerPrivateMembers::on_interrupt( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has been interrupted on the server" << std::endl;
	if( !removeConnection(hdl) ) std::cout << "Couldn't find connection to remove" << std::endl;
}

void reverseshell::ServerPrivateMembers::on_message( websocketpp::connection_hdl hdl, server_type::message_ptr pMessage )
{
	switch( static_cast<Client::MessageType>(pMessage->get_payload()[0]) )
	{
		case Client::MessageType::StdOut:
			std::cout << "Server received stdout message '" << &pMessage->get_payload()[1] << "'" << std::endl;
			break;
		case Client::MessageType::StdErr:
			std::cerr << "Server received stderr message '" << &pMessage->get_payload()[1] << "'" << std::endl;
			break;
		default:
			std::cerr << "Server received unknown message type '" << pMessage->get_payload() << "'" << std::endl;
	}
}

std::shared_ptr<websocketpp::lib::asio::ssl::context> reverseshell::ServerPrivateMembers::on_tls_init( websocketpp::connection_hdl hdl )
{
	std::cout << "TLS is being initiated in the server" << std::endl;
	namespace asio=websocketpp::lib::asio;
	auto pSSLContext=std::make_shared<websocketpp::lib::asio::ssl::context>(asio::ssl::context::tlsv12);
	// Try and copy the "intermediate" SSL (actually TLS) settings from
	// https://mozilla.github.io/server-side-tls/ssl-config-generator/. There's
	// not an OpenSSL option but you can infer most of the options from the other
	// configurations.
	pSSLContext->set_options( asio::ssl::context::default_workarounds |
	                        asio::ssl::context::no_sslv2 |
	                        asio::ssl::context::no_sslv3 |
	                        asio::ssl::context::single_dh_use );
	SSL_CTX_set_cipher_list( pSSLContext->native_handle(), "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:"
		"ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:"
		"DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256:"
		"ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES256-SHA384:"
		"ECDHE-ECDSA-AES256-SHA:ECDHE-RSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-RSA-AES256-SHA256:"
		"DHE-RSA-AES256-SHA:ECDHE-ECDSA-DES-CBC3-SHA:ECDHE-RSA-DES-CBC3-SHA:EDH-RSA-DES-CBC3-SHA:AES128-GCM-SHA256:"
		"AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:DES-CBC3-SHA:!DSS" );
	//pSSLContext->set_password_callback( websocketpp::lib::bind( &server::get_password, this ) );
	if( !certificateChainFileName_.empty() ) pSSLContext->use_certificate_chain_file( certificateChainFileName_ );
	if( !privateKeyFileName_.empty() ) pSSLContext->use_private_key_file( privateKeyFileName_, asio::ssl::context::pem );
	if( !verifyFileName_.empty() )
	{
		pSSLContext->set_verify_mode( asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert );
		pSSLContext->load_verify_file( verifyFileName_ );
		//pSSLContext->set_verify_callback( std::bind( &ServerPimple::verify_certificate, this, std::placeholders::_1, std::placeholders::_2 ) );
	}
	else pSSLContext->set_verify_mode( asio::ssl::verify_none );
	if( !diffieHellmanParamsFileName_.empty() ) pSSLContext->use_tmp_dh_file( diffieHellmanParamsFileName_ );

	return pSSLContext;
}
