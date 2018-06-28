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
#include "reverseshell/Connection.h"

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio.hpp>
#include <pstream.h>

//
// Declaration of the pimple
//
namespace reverseshell
{
	class ConnectionPrivateMembers
	{
	public:
		std::function<void(Connection::MessageType, const char*, size_t)> messageCallback_;
		std::function<void(const char*, size_t)> sendFunction_;

		ConnectionPrivateMembers( std::function<void(const char*, size_t)> sendFunction );
	};
}

reverseshell::Connection::Connection()
	: pImple_( nullptr )
{
	// No operation.
}

reverseshell::Connection::Connection( std::function<void(const char*, size_t)> sendFunction )
	: pImple_( std::make_shared<ConnectionPrivateMembers>( sendFunction ) )
{
	// No operation.
}

reverseshell::Connection::~Connection()
{
	// No operation
}

void reverseshell::Connection::send( const char* message, size_t size )
{
	if( !pImple_ ) throw std::runtime_error("reverseshell::Connection::send() - Invalid Connection object");
	pImple_->sendFunction_( message, size );
}

void reverseshell::Connection::send( const std::string& message )
{
	if( !pImple_ ) throw std::runtime_error("reverseshell::Connection::send() - Invalid Connection object");
	pImple_->sendFunction_( message.data(), message.size() );
}

void reverseshell::Connection::setMessageCallback( std::function<void(Connection::MessageType, const char*, size_t)> callback )
{
	if( !pImple_ ) throw std::runtime_error("reverseshell::Connection::setStdOutCallback() - Invalid Connection object");
	pImple_->messageCallback_=callback;
}

const std::function<void(reverseshell::Connection::MessageType, const char*, size_t)>& reverseshell::Connection::callback()
{
	return pImple_->messageCallback_;
}

reverseshell::ConnectionPrivateMembers::ConnectionPrivateMembers( std::function<void(const char*, size_t)> sendFunction )
	: sendFunction_( sendFunction )
{
	messageCallback_=[]( Connection::MessageType type, const char* message, size_t size ){
		switch( type )
		{
			case Connection::MessageType::StdOut:
				std::cout.write( message, size );
				std::cout.flush();
				break;
			case Connection::MessageType::StdErr:
				std::cerr.write( message, size );
				std::cerr.flush();
				break;
		} // end of switch on MessageType
	};
}
