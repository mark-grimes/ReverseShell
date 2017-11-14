#include "reverseshell/Client.h"
#include "catch.hpp"

SCENARIO( "Test that reverseshell::Client works as expected", "[Client]" )
{
	GIVEN( "An instance of a Client" )
	{
		reverseshell::Client anInstance;

		WHEN( "Testing some result" )
		{
			CHECK( 1+1 == 3 );
		}
	} // end of 'GIVEN "An instance of a Client"'
} // end of 'SCENARIO ... Client'
