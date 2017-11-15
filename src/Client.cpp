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
#include "reverseshell/Client.h"

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio.hpp>

//
// Declaration of the pimple
//
namespace reverseshell
{
	class ClientPrivateMembers
	{
	public:
		typedef websocketpp::client<websocketpp::config::asio_tls> client_type;
		enum class State { Ready, Connecting, Connected, Disconnecting };
		client_type client_;
		State state_;
		mutable std::mutex stateMutex_;
		client_type::connection_ptr connection_;
		std::string certificateChainFileName_;
		std::string privateKeyFileName_;
		std::string verifyFileName_;
		std::string diffieHellmanParamsFileName_;

		void on_open( websocketpp::connection_hdl hdl );
		void on_close( websocketpp::connection_hdl hdl );
		void on_interrupt( websocketpp::connection_hdl hdl );
		std::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init( websocketpp::connection_hdl hdl );
	};
}

reverseshell::Client::Client()
	: pImple_( new ClientPrivateMembers )
{
	pImple_->state_=ClientPrivateMembers::State::Ready;
	pImple_->client_.set_access_channels(websocketpp::log::alevel::none);
	//pImple_->client_.set_error_channels(websocketpp::log::elevel::all ^ websocketpp::log::elevel::info);
	//pImple_->client_.set_error_channels(websocketpp::log::elevel::none);
	pImple_->client_.set_error_channels(websocketpp::log::elevel::all);
	pImple_->client_.set_tls_init_handler( std::bind( &ClientPrivateMembers::on_tls_init, pImple_.get(), std::placeholders::_1 ) );
	pImple_->client_.init_asio();
	pImple_->client_.set_open_handler( std::bind( &ClientPrivateMembers::on_open, pImple_.get(), std::placeholders::_1 ) );
	pImple_->client_.set_close_handler( std::bind( &ClientPrivateMembers::on_close, pImple_.get(), std::placeholders::_1 ) );
	pImple_->client_.set_interrupt_handler( std::bind( &ClientPrivateMembers::on_interrupt, pImple_.get(), std::placeholders::_1 ) );
}

reverseshell::Client::Client( Client&& otherClient ) noexcept
	: pImple_( std::move(otherClient.pImple_) )
{
	// No operation, everything done in initialiser list
}

reverseshell::Client::~Client()
{
	// No operation
}

void reverseshell::Client::connect( const std::string& URI )
{
	{ // Block to limit scope of stateLock
		std::unique_lock<std::mutex> stateLock(pImple_->stateMutex_);
		// If in the process of disconnecting, give a little time for it to take effect
		for( size_t tries=0; pImple_->state_==ClientPrivateMembers::State::Disconnecting && tries<10; ++tries )
		{
			stateLock.unlock();
			std::this_thread::sleep_for( std::chrono::milliseconds(50) );
			stateLock.lock();
		}

		pImple_->state_=ClientPrivateMembers::State::Connecting;
		websocketpp::lib::error_code errorCode;
		pImple_->connection_=pImple_->client_.get_connection( URI, errorCode );
		if( errorCode.value()!=0 ) throw std::runtime_error( "Unable to get the websocketpp connection - "+errorCode.message() );

		if( errorCode )
		{
			pImple_->connection_=nullptr;
			pImple_->state_=ClientPrivateMembers::State::Ready;
			pImple_->client_.get_alog().write(websocketpp::log::alevel::app,errorCode.message());
			throw std::runtime_error( errorCode.message() );
		}

		pImple_->client_.connect( pImple_->connection_ );
	} // Need to release stateLock before blocking in the run() call
	pImple_->client_.run();
}

void reverseshell::Client::disconnect()
{
	std::unique_lock<std::mutex> stateLock(pImple_->stateMutex_);
	// If in the process of connecting, give a little time for it to take effect
	for( size_t tries=0; pImple_->state_==ClientPrivateMembers::State::Connecting && tries<10; ++tries )
	{
		stateLock.unlock();
		std::this_thread::sleep_for( std::chrono::milliseconds(50) );
		stateLock.lock();
	}

	if( pImple_->state_==ClientPrivateMembers::State::Connected )
	{
		pImple_->state_=ClientPrivateMembers::State::Disconnecting;
		if( pImple_->connection_ )
		{
			pImple_->connection_->close( websocketpp::close::status::normal, "Had enough. Bye." );
			pImple_->connection_=nullptr;
		}
	}
	// Don't worry if already disconnected, don't treat it as an error
}

void reverseshell::Client::setCertificateChainFile( const std::string& filename )
{
	pImple_->certificateChainFileName_=filename;
}

void reverseshell::Client::setPrivateKeyFile( const std::string& filename )
{
	pImple_->privateKeyFileName_=filename;
}

void reverseshell::Client::setVerifyFile( const std::string& filename )
{
	pImple_->verifyFileName_=filename;
}

void reverseshell::ClientPrivateMembers::on_open( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has opened on the client" << std::endl;
	std::lock_guard<std::mutex> stateLock(stateMutex_);
	state_=State::Connected;
}

void reverseshell::ClientPrivateMembers::on_close( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has closed on the client" << std::endl;
	std::lock_guard<std::mutex> stateLock(stateMutex_);
	state_=State::Ready;
}

void reverseshell::ClientPrivateMembers::on_interrupt( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has been interrupted on the client" << std::endl;
	std::lock_guard<std::mutex> stateLock(stateMutex_);
	state_=State::Ready;
}

std::shared_ptr<websocketpp::lib::asio::ssl::context> reverseshell::ClientPrivateMembers::on_tls_init( websocketpp::connection_hdl hdl )
{
	std::cout << "TLS is being initiated in the client" << std::endl;
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
