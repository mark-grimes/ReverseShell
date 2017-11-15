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


		ServerPrivateMembers();
		bool removeConnection( websocketpp::connection_hdl hdl );
		void on_http( websocketpp::connection_hdl hdl );
		void on_open( websocketpp::connection_hdl hdl );
		void on_close( websocketpp::connection_hdl hdl );
		void on_interrupt( websocketpp::connection_hdl hdl );
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
		throw std::runtime_error( "reverseshell::Server listen error: "+errorCode.message() );
	}

	pImple_->server_.start_accept();
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

	std::lock_guard<std::mutex> currentConnectionsLock( pImple_->currentConnectionsMutex_ );
	std::cout << "Stopping server with " << pImple_->currentConnections_.size() << " open connections" << std::endl;
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
	throw std::runtime_error("Not implemented yet ("+std::string(__FILE__)+":"+std::to_string(__LINE__)+")");
}

void reverseshell::Server::setPrivateKeyFile( const std::string& filename )
{
	throw std::runtime_error("Not implemented yet ("+std::string(__FILE__)+":"+std::to_string(__LINE__)+")");
}

void reverseshell::Server::setVerifyFile( const std::string& filename )
{
	throw std::runtime_error("Not implemented yet ("+std::string(__FILE__)+":"+std::to_string(__LINE__)+")");
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
	std::cout << "Connection has opened" << std::endl;
	std::lock_guard<std::mutex> myMutex( currentConnectionsMutex_ );
	currentConnections_.emplace_back( hdl );
}

void reverseshell::ServerPrivateMembers::on_close( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has closed" << std::endl;
	if( !removeConnection(hdl) ) std::cout << "Couldn't find connection to remove" << std::endl;
}

void reverseshell::ServerPrivateMembers::on_interrupt( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has been interrupted" << std::endl;
	if( !removeConnection(hdl) ) std::cout << "Couldn't find connection to remove" << std::endl;
}

std::shared_ptr<websocketpp::lib::asio::ssl::context> reverseshell::ServerPrivateMembers::on_tls_init( websocketpp::connection_hdl hdl )
{
	std::cout << "TLS is being initiated" << std::endl;
	namespace asio=websocketpp::lib::asio;
	return std::make_shared<websocketpp::lib::asio::ssl::context>(asio::ssl::context::tlsv12);
}