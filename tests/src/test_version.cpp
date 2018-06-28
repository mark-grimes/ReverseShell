#include "reverseshell/version.h"
#include <string>
#include "catch.hpp"

bool isHexChar( char character )
{
	return (character == '0') || (character == '1') || (character == '2') || (character == '3') || (character == '4')
	    || (character == '5') || (character == '6') || (character == '7') || (character == '8') || (character == '9')
	    || (character == 'a') || (character == 'b') || (character == 'c') || (character == 'd') || (character == 'e') || (character == 'f')
	    || (character == 'A') || (character == 'B') || (character == 'C') || (character == 'D') || (character == 'E') || (character == 'F');
}

SCENARIO( "Test that version gives the correct git information", "[version]" )
{
	WHEN( "Test there are none zero values for the git hash and git describe" )
	{
		CHECK( reverseshell::version::GitDescribe != nullptr );
		CHECK( std::char_traits<char>::length(reverseshell::version::GitDescribe) > 0 ); // Not sure how to check this consistently
		CHECK( reverseshell::version::GitHash[40] == 0 );
		CHECK( std::char_traits<char>::length(reverseshell::version::GitHash) == 40 ); // Check there are no nulls half way through
	}
	WHEN( "Checking the git hash is all valid hex characters" )
	{
		size_t index;
		for( index=0; index<40; ++index )
		{
			if( !isHexChar(reverseshell::version::GitHash[index]) ) break;
		}
		INFO( std::string("The git hash '")+reverseshell::version::GitHash+"' is not valid hex characters" );
		CHECK( index == 40 ); // make sure the loop completed all the way
	} // end of WHEN checking all characters are hex
} // end of 'SCENARIO ... SomeClass'
