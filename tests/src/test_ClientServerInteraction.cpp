#include "reverseshell/Client.h"
#include "reverseshell/Server.h"
#include "reverseshell/Connection.h"
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
	void listen( size_t port ) { server_.listen(port); }
	void run()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		threadHasStarted_=false;

		thread_=std::thread( std::bind( &ServerThread::threadLoop, this ) );

		// Block until the thread hass started
		condition_.wait( lock, [this](){ return threadHasStarted_; } );
	}
	void setCertificateChainFile( const std::string& filename ) { server_.setCertificateChainFile( filename ); }
	void setPrivateKeyFile( const std::string& filename ) { server_.setPrivateKeyFile( filename ); }
	void setNewConnectionCallback( std::function<void(reverseshell::Connection& connection)> connection ) { server_.setNewConnectionCallback(connection); }
protected:
	void threadLoop()
	{
		try
		{
			// Unlock the main thread
			{
				std::unique_lock<std::mutex> lock(mutex_);
				threadHasStarted_=true;
				condition_.notify_all();
			}
			server_.run();
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
	ClientThread() {}
	~ClientThread()
	{
		client_.disconnect();
		thread_.join();
	}
	void setVerifyFile( const std::string& filename ) { client_.setVerifyFile( filename ); }
	void connect( const std::string& uri ) { client_.connect( uri ); }
	void run()
	{
		thread_=std::thread( [this](){
			try { client_.run(); }
			catch( const std::runtime_error& error ){ std::cerr << "Exception while connecting client: " << error.what() << std::endl; }
			catch(...){ std::cerr << "Unknown exception while connecting client" << std::endl; }
		} );
	}
	void send( const std::string& message ) { client_.send(message); }
protected:
	reverseshell::Client client_;
	std::thread thread_;
};

SCENARIO( "Test that reverseshell::Client and reverseshell::Server can interact properly", "[Client][Server]" )
{
	GIVEN( "An instance of a Server" )
	{
		ServerThread server;
		server.setCertificateChainFile(reverseshelltests::testinputs::testFileDirectory+"/serverA_cert.pem");
		server.setPrivateKeyFile(reverseshelltests::testinputs::testFileDirectory+"/serverA_key.pem");


		WHEN( "Starting the server on a separate thread" )
		{
			CHECK_NOTHROW( server.listen( 9000 ) );
			CHECK_NOTHROW( server.run() );
		}
		WHEN( "Starting the server and connecting the client to it" )
		{
			CHECK_NOTHROW( server.listen( 9001 ) );
			CHECK_NOTHROW( server.run() );
			ClientThread client;
			CHECK_NOTHROW( client.setVerifyFile(reverseshelltests::testinputs::testFileDirectory+"/authorityA_cert.pem") );
			CHECK_NOTHROW( client.connect( "wss://localhost:9001/" ) );
			CHECK_NOTHROW( client.run() );
			// TODO add proper wait conditions in the destructors so that the process can finish without an explicit wait here
			std::this_thread::sleep_for( std::chrono::seconds(1) );
		}
		WHEN( "Getting the remote shell to 'ls' the test files directory" )
		{
			// The test file directory is one where I know what the contents is, and
			// its absolute location. Perform an ls on it to make sure the remote shell
			// is doing what I say.
			std::string remoteCout;
			std::string remoteCerr;
			auto coutFunc=[&remoteCout](const char* message, size_t size){ remoteCout.append( message, size ); };
			auto cerrFunc=[&remoteCerr](const char* message, size_t size){ remoteCerr.append( message, size ); };
			CHECK_NOTHROW( server.setNewConnectionCallback(
					[&coutFunc,&cerrFunc]( reverseshell::Connection& connection )
					{
						connection.setStdOutCallback( coutFunc );
						connection.setStdErrCallback( cerrFunc );
						connection.send( "cd "+reverseshelltests::testinputs::testFileDirectory+"\n" );
						connection.send( "ls\n" );
					} ) );

			CHECK_NOTHROW( server.listen( 9002 ) );
			CHECK_NOTHROW( server.run() );
			ClientThread client;
			CHECK_NOTHROW( client.setVerifyFile(reverseshelltests::testinputs::testFileDirectory+"/authorityA_cert.pem") );
			CHECK_NOTHROW( client.connect( "wss://localhost:9002/" ) );
			CHECK_NOTHROW( client.run() );


			// TODO add proper wait conditions in the destructors so that the process can finish without an explicit wait here
			std::this_thread::sleep_for( std::chrono::seconds(1) );
			CHECK( remoteCerr.empty() );
			CHECK( remoteCout == "authorityA_cert.pem\n"
			                     "authorityB_cert.pem\n"
			                     "serverA_cert_expired.pem\n"
			                     "serverA_cert.pem\n"
			                     "serverA_key.pem\n"
			                     "serverB_cert_expired.pem\n"
			                     "serverB_cert.pem\n"
			                     "serverB_key.pem\n" );
		}
	} // end of 'GIVEN "An instance of a Server"'
} // end of 'SCENARIO ... Client Server interaction'
