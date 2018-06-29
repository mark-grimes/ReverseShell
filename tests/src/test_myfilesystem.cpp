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
} // end of 'SCENARIO ... myfilesystem::canonical'
