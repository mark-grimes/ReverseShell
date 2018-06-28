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
#include "pstream.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <array>
#include <chrono>
#include <clara.hpp>
#include "reverseshell/Server.h"
#include "reverseshell/Connection.h"

const std::string* pScriptDirectory=nullptr;
const std::string* pLogOutputDirectory=nullptr;

struct ConnectionState
{
	std::string hostname;
	bool suppressPrefix=false;
	reverseshell::Connection connection;
	ConnectionState( reverseshell::Connection& connection ) : connection(connection) {}
};

/** @brief Utility to easily print the time to a std::ostream */
struct PrintTime
{
	PrintTime( std::chrono::system_clock::time_point time=std::chrono::system_clock::now() ) : time_(std::chrono::system_clock::to_time_t(time)) {}
	std::time_t time_;
};
std::ostream& operator<<( std::ostream& stream, const PrintTime& time )
{
	stream << std::put_time( std::localtime(&time.time_), "%F %T" );
	return stream;
}

void printMessage( std::ostream& stream, std::shared_ptr<ConnectionState> pState, reverseshell::Connection::MessageType type, const char* message, size_t size )
{
	if( !pState->suppressPrefix ) stream << "[" << PrintTime() << "] " << pState->hostname << (type==reverseshell::Connection::MessageType::StdOut ? " stdout: " : " stderr: ");
	pState->suppressPrefix=true;

	size_t lastPosition=0;
	for( size_t currentPosition=0; currentPosition<size; ++currentPosition )
	{
		if( message[currentPosition]=='\n' )
		{
			stream.write( &message[lastPosition], currentPosition-lastPosition+1 );
			if( currentPosition+1<size )
			{
				stream << "[" << PrintTime() << "] " << pState->hostname << (type==reverseshell::Connection::MessageType::StdOut ? " stdout: " : " stderr: ");
			}
			lastPosition=currentPosition+1;
		}
	}
	// Write anything not already written
	if( lastPosition<size ) stream.write( &message[lastPosition], size-lastPosition );
	else pState->suppressPrefix=false;
}

void messageCallback( std::shared_ptr<ConnectionState> pState, reverseshell::Connection::MessageType type, const char* message, size_t size )
{
	if( pState->hostname.empty() )
	{
		if( size>0 && type==reverseshell::Connection::MessageType::StdOut )
		{
			if( message[size-1]=='\n' ) pState->hostname=std::string( message, size-1 );
			else pState->hostname=std::string( message, size );
		}
		std::cout << "[" << PrintTime() << "] "  << "Connection has hostname '" << pState->hostname << "'" << std::endl;

		// See if there is a script file for this hostname
		std::string scriptFilename(*pScriptDirectory + "/" + pState->hostname + ".sh");
		std::ifstream inputScript( scriptFilename );
		if( !inputScript.is_open() )
		{
			std::cout << "[" << PrintTime() << "] "  << "Unable to open script '" << scriptFilename << "'" << std::endl;
			scriptFilename= *pScriptDirectory + "/default.sh";
			inputScript.open( scriptFilename );
			if( !inputScript.is_open() )
			{
				std::cout << "[" << PrintTime() << "] "  << "Unable to open script '" << scriptFilename << "', closing connection" << std::endl;
				pState->connection.send("");
			}
		}

		if( inputScript.is_open() )
		{
			std::array<char,256> buffer;
			while( !inputScript.fail() )
			{
				inputScript.read( buffer.data(), buffer.size() );
				pState->connection.send( buffer.data(), inputScript.gcount() );
			}
			std::cout << "[" << PrintTime() << "] "  << "Finished sending script to '" << pState->hostname << "'" << std::endl;
			pState->connection.send("");
		}
	}
	else // Already have a hostname, so handle as normal
	{
		// Even errors are written to stdout, because it is an error on the client, not on the server
		printMessage( std::cout, pState, type, message, size );
	}
}

void newConnectionCallback( reverseshell::Connection& connection )
{
	auto pState=std::make_shared<ConnectionState>( connection );
	std::cout << "Attempting to get hostname for connection" << std::endl;
	connection.setMessageCallback( std::bind( &messageCallback, pState, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ) );
	connection.send("hostname\n");
}


int main( int argc, char* argv[] )
{
	size_t port=443;
	std::string certFilename;
	std::string keyFilename;
	bool printHelp=false;
	std::string scriptDirectory("./");
	std::string logOutputDirectory("./");
	pScriptDirectory=&scriptDirectory;
	pLogOutputDirectory=&logOutputDirectory;

	auto cli=clara::Opt( certFilename, "filename" )
	             ["-c"]["--cert"]["--certificate"]
	             ("The filename of the certificate to present to clients")
	       | clara::Opt( keyFilename, "filename" )
	             ["-k"]["--key"]
	             ("The filename of the key for the certificate")
	       | clara::Opt( port, "port number" )
	             ["-p"]["--port"]
	             ("The port to listen on for connections")
	       | clara::Opt( scriptDirectory, "input script directory" )
	             ["-s"]["--script-dir"]
	             ("The directory to look for update scripts in")
	       | clara::Help( printHelp );

	auto parseResult=cli.parse( clara::Args( argc, argv ) );
	if( !parseResult )
	{
		std::cerr << "Error while parsing the command line: " << parseResult.errorMessage() << std::endl;
		// print the usage
		std::cerr << cli << std::endl;
		return -1;
	}
	if( printHelp )
	{
		std::cout << cli << std::endl;
		return 0;
	}

	try
	{
		reverseshell::Server server;
		server.setNewConnectionCallback( newConnectionCallback );

		if( !certFilename.empty() ) server.setCertificateChainFile( certFilename );
		if( !keyFilename.empty() ) server.setPrivateKeyFile( keyFilename );
		std::cout << "Listening on port " << port << std::endl;
		server.listen( port );
		server.run();
	}
	catch( const std::exception& error )
	{
		std::cerr << "Received exception: " << error.what() << std::endl;
		return -1;
	}
	catch( ... )
	{
		std::cerr << "Received unknown exception." << std::endl;
		return -1;
	}


	return 0;
}
