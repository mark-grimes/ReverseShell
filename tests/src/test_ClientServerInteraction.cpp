#include "reverseshell/Client.h"
#include "reverseshell/Server.h"
#include <thread>
#include <iostream>
#include <condition_variable>
#include "reverseshelltests/testinputs.h"
#include "catch.hpp"

/** @brief Wrapper around reverseshell::Server that starts the server on a separate thread so that it is non blocking. */
class ServerThread
{
public:
	ServerThread() : threadHasStarted_(false) {}
	~ServerThread() { server_.stop(); thread_.join(); }
	void listen( size_t port )
	{
		std::unique_lock<std::mutex> lock(mutex_);
		threadHasStarted_=false;

		thread_=std::thread( std::bind( &ServerThread::threadLoop, this, port ) );

		// Block until the thread hass started
		condition_.wait( lock, [this](){ return threadHasStarted_; } );
	}
	void setCertificateChainFile( const std::string& filename ) { server_.setCertificateChainFile( filename ); }
	void setPrivateKeyFile( const std::string& filename ) { server_.setPrivateKeyFile( filename ); }
protected:
	void threadLoop( size_t port )
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
	}
	reverseshell::Server server_;
	std::condition_variable condition_;
	std::mutex mutex_;
	bool threadHasStarted_;
	std::thread thread_;
};

/** @brief Wrapper around reverseshell::Client that connects on a separate thread */
class ClientThread
{
public:
	ClientThread() : threadHasStarted_(false) {}
	~ClientThread()
	{
		client_.disconnect();
		thread_.join();
	}
	void setVerifyFile( const std::string& filename ) { client_.setVerifyFile( filename ); }
	void connect( const std::string& uri )
	{
		std::unique_lock<std::mutex> lock(mutex_);
		threadHasStarted_=false;

		thread_=std::thread( std::bind( &ClientThread::threadLoop, this, uri ) );

		// Block until the thread has started
		condition_.wait( lock, [this](){ return threadHasStarted_; } );
	}
protected:
	void threadLoop( const std::string& uri )
	{
		try
		{
			// Unlock the main thread
			{
				std::unique_lock<std::mutex> lock(mutex_);
				threadHasStarted_=true;
				condition_.notify_all();
			}
			client_.connect( uri );
		}
		catch( const std::runtime_error& error ){ std::cerr << "Exception while connecting client: " << error.what() << std::endl; }
		catch(...){ std::cerr << "Unknown exception while connecting client" << std::endl; }
	}
	reverseshell::Client client_;
	std::condition_variable condition_;
	std::mutex mutex_;
	bool threadHasStarted_;
	std::thread thread_;
};

SCENARIO( "Test that reverseshell::Client and reverseshell::Server can interact properly", "[Client][Server]" )
{
	GIVEN( "An instance of a Server" )
	{
		ServerThread server;
		server.setCertificateChainFile(reverseshelltests::testinputs::testFileDirectory+"/server_cert.pem");
		server.setPrivateKeyFile(reverseshelltests::testinputs::testFileDirectory+"/server_key.pem");


		WHEN( "Starting the server on a separate thread" )
		{
			CHECK_NOTHROW( server.listen( 9000 ) );
		}
		WHEN( "Starting the server and connecting the client to it" )
		{
			CHECK_NOTHROW( server.listen( 9001 ) );
			ClientThread client;
			CHECK_NOTHROW( client.setVerifyFile(reverseshelltests::testinputs::testFileDirectory+"/authority_cert.pem") );
			CHECK_NOTHROW( client.connect( "wss://localhost:9001/" ) );
			std::this_thread::sleep_for( std::chrono::seconds(1) );
		}
	} // end of 'GIVEN "An instance of a Server"'
} // end of 'SCENARIO ... Client Server interaction'
