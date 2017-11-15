#include "reverseshell/Client.h"
#include "reverseshell/Server.h"
#include <thread>
#include <iostream>
#include <condition_variable>
#include "reverseshelltests/testinputs.h"
#include "catch.hpp"

/** @brief Starts the server on a separate thread so that it is non blocking. Stops the server on destruction. */
class StartServer
{
public:
	StartServer( reverseshell::Server& server, size_t port )
		: server_(server),
		  threadHasStarted_(false),
		  thread_( [this,port]()
			{
				try
				{
					// Unlock the main thread
					{
						std::unique_lock<std::mutex> lock(mutex_);
						threadHasStarted_=true;
						condition_.notify_all();
					}
					server_.listen(port);
				}
				catch( const std::runtime_error& error ){ std::cerr << "Exception while starting server: " << error.what() << std::endl; }
				catch(...){ std::cerr << "Unknown exception while starting server" << std::endl; }
			} )
	{
		// Block until the thread has started, otherwise the "stop()" could get lost
		{
			std::unique_lock<std::mutex> lock(mutex_);
			condition_.wait( lock, [this](){ return threadHasStarted_; } );
		}
	}
	~StartServer()
	{
		server_.stop();
		thread_.join();
	}
protected:
	reverseshell::Server& server_;
	std::condition_variable condition_;
	std::mutex mutex_;
	bool threadHasStarted_;
	std::thread thread_;
};

SCENARIO( "Test that reverseshell::Client and reverseshell::Server can interact properly", "[Client][Server]" )
{
	GIVEN( "An instance of a Server" )
	{
		reverseshell::Server server;
		server.setCertificateChainFile(reverseshelltests::testinputs::testFileDirectory+"/server_cert.pem");
		server.setPrivateKeyFile(reverseshelltests::testinputs::testFileDirectory+"/server_key.pem");


		WHEN( "Starting the server on a separate thread" )
		{
			// This just makes sure it doesn't throw and doesn't block
			StartServer serverGuard( server, 9000 );
		}
		WHEN( "Starting the server and connecting the client to it" )
		{
			StartServer serverGuard( server, 9001 );
			reverseshell::Client client;
			client.setVerifyFile(reverseshelltests::testinputs::testFileDirectory+"/authority_cert.pem");
			std::this_thread::sleep_for( std::chrono::seconds(2) );
			CHECK_NOTHROW( client.connect( "wss://localhost:9001/" ) );
		}
	} // end of 'GIVEN "An instance of a Server"'
} // end of 'SCENARIO ... Client Server interaction'
