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
#include <deque>
#include <mutex>
#include <thread>
#include <clara.hpp>
#include "reverseshell/Server.h"
#include "reverseshell/Connection.h"

std::deque<reverseshell::Connection> connections;
std::mutex connectionsMutex;
std::thread inputThread;

void userInputLoop()
{
	while( true )
	{
		try
		{
			reverseshell::Connection connection;
			{
				std::lock_guard<std::mutex> connectionsLock(connectionsMutex);
				if( connections.empty() ) break;
				connection=connections.front();
				connections.pop_front();
				std::cout << "Connecting terminal to next connection (" << connections.size() << " more connection(s) available)"  << std::endl;
			}

			std::string input;
			while( true )
			{
				input.clear();
				std::cout << "\nconn> ";
				std::cin >> input;
				if( std::cin.eof() )
				{
					std::cout << "\nGot EOF" << std::endl;
					std::cin.clear();
					connection.send("");
					break;
				}
				connection.send(input+"\n");
			}
		}
		catch( const std::exception& error )
		{
			std::cerr << "Exception from connection: " << error.what() << std::endl;
		}
	}  // End of loop over connections
	std::cout << "Input loop finished" << std::endl;
}

void newConnectionCallback( reverseshell::Connection& connection )
{
	std::cout << "New connection (" << connections.size()+1 << " connection(s) available)"  << std::endl;
	std::lock_guard<std::mutex> connectionsLock(connectionsMutex);
	connections.emplace_back( connection );
	if( connections.size()==1 )
	{
		if( inputThread.joinable() ) inputThread.join();
		inputThread=std::thread( userInputLoop );
	}
}


int main( int argc, char* argv[] )
{
	size_t port=433;
	std::string certFilename;
	std::string keyFilename;
	bool printHelp=false;

	auto cli=clara::Opt( certFilename, "filename" )
	             ["-c"]["--cert"]["--certificate"]
	             ("The filename of the certificate to present to clients")
	       | clara::Opt( keyFilename, "filename" )
	             ["-k"]["--key"]
	             ("The filename of the key for the certificate")
	       | clara::Opt( port, "port number" )
	             ["-p"]["--port"]
	             ("The port to listen on for connections")
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
