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
#include <clara.hpp>
#include "reverseshell/Client.h"

int main( int argc, char* argv[] )
{
	std::string serverUri;
	std::string verifyCertFilename;
	bool printHelp=false;

	auto cli=clara::Opt( verifyCertFilename, "filename" )
	             ["-v"]["--verify"]
	             ("The filename of the Certificate Authority certificate to verify the server against")
	       | clara::Arg( serverUri, "server URI" )
	             ("The remote server URI/URL to connect to")
	             .required()
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
		reverseshell::Client client;

		if( !verifyCertFilename.empty() ) client.setVerifyFile( verifyCertFilename );
		client.connect( serverUri );
		std::cout << "Connecting to " << serverUri << std::endl;
		client.run();
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
