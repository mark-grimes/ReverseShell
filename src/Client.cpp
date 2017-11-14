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
		client_type client_;

		void on_open( websocketpp::connection_hdl hdl );
		void on_close( websocketpp::connection_hdl hdl );
		void on_interrupt( websocketpp::connection_hdl hdl );
		std::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init( websocketpp::connection_hdl hdl );
	};
}

reverseshell::Client::Client()
	: pImple_( new ClientPrivateMembers )
{
	pImple_->client_.set_access_channels(websocketpp::log::alevel::none);
	//pImple_->client_.set_error_channels(websocketpp::log::elevel::all ^ websocketpp::log::elevel::info);
	pImple_->client_.set_error_channels(websocketpp::log::elevel::none);
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
	websocketpp::lib::error_code errorCode;
	auto pWebPPConnection=pImple_->client_.get_connection( URI, errorCode );
	if( errorCode.value()!=0 ) throw std::runtime_error( "Unable to get the websocketpp connection - "+errorCode.message() );

	if( errorCode )
	{
		pImple_->client_.get_alog().write(websocketpp::log::alevel::app,errorCode.message());
		throw std::runtime_error( errorCode.message() );
	}

	pImple_->client_.connect( pWebPPConnection );
	pImple_->client_.run();
}

void reverseshell::Client::setCertificateChainFile( const std::string& filename )
{
	throw std::runtime_error("Not implemented yet ("+std::string(__FILE__)+":"+std::to_string(__LINE__)+")");
}

void reverseshell::Client::setPrivateKeyFile( const std::string& filename )
{
	throw std::runtime_error("Not implemented yet ("+std::string(__FILE__)+":"+std::to_string(__LINE__)+")");
}

void reverseshell::Client::setVerifyFile( const std::string& filename )
{
	throw std::runtime_error("Not implemented yet ("+std::string(__FILE__)+":"+std::to_string(__LINE__)+")");
}

void reverseshell::ClientPrivateMembers::on_open( websocketpp::connection_hdl hdl )
{
}

void reverseshell::ClientPrivateMembers::on_close( websocketpp::connection_hdl hdl )
{
}

void reverseshell::ClientPrivateMembers::on_interrupt( websocketpp::connection_hdl hdl )
{
	std::cout << "Connection has been interrupted" << std::endl;
}

std::shared_ptr<websocketpp::lib::asio::ssl::context> reverseshell::ClientPrivateMembers::on_tls_init( websocketpp::connection_hdl hdl )
{
	std::cout << "TLS is being initiated" << std::endl;
	return nullptr;
}
