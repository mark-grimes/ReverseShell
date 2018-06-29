#include "myfilesystem.h"
#include "reverseshelltests/testinputs.h"
#include "catch.hpp"

SCENARIO( "Test that myfilesystem::canonical works as expected", "[myfilesystem]" )
{
	WHEN( "Checking the testinputs::testFileDirectory directory" )
	{
		const std::string& testFileDirectory=reverseshelltests::testinputs::testFileDirectory;
		std::string result;
		REQUIRE_NOTHROW( result=myfilesystem::canonical(testFileDirectory) );
		// testFileDirectory has a slash on the end, so knock that off for the comparison
		REQUIRE( result==testFileDirectory.substr(0,testFileDirectory.size()-1) );
	}
	WHEN( "Testing with a made up directory name" )
	{
		std::string result;
		REQUIRE_THROWS( result=myfilesystem::canonical("/blah/asdf/asdfweoij/../aeivne") );
	}
	WHEN( "Testing with .. and . in the path" )
	{
		const std::string& testFileDirectory=reverseshelltests::testinputs::testFileDirectory;
		std::string result;
		REQUIRE_NOTHROW( result=myfilesystem::canonical(testFileDirectory+"/.././") );
		// The above line should knock off the "/testdata/" part, i.e. 10 characters
		REQUIRE( result==testFileDirectory.substr(0,testFileDirectory.size()-10) );
	}
} // end of 'SCENARIO ... SomeClass'
