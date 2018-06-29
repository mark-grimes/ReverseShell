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
#include "reverseshell/tools.h"
#include "catch.hpp"

SCENARIO( "Test that reverseshell::tools::sanitiseHostname works as expected", "[tools]" )
{
	WHEN( "Testing valid hostnames" )
	{
		std::string result;
		REQUIRE_NOTHROW( result=reverseshell::tools::sanitiseHostname("www.google.com") );
		CHECK( result=="www.google.com" );
		REQUIRE_NOTHROW( result=reverseshell::tools::sanitiseHostname("www.gOOgle.com") );
		CHECK( result=="www.gOOgle.com" );
		REQUIRE_NOTHROW( result=reverseshell::tools::sanitiseHostname("www.bbc.co.uk") );
		CHECK( result=="www.bbc.co.uk" );
	}
	WHEN( "Testing with a newline on the end" )
	{
		std::string result;
		REQUIRE_NOTHROW( result=reverseshell::tools::sanitiseHostname("www.google.com\n") );
		CHECK( result=="www.google.com" );
		REQUIRE_NOTHROW( result=reverseshell::tools::sanitiseHostname("www.gOOgle.com\n") );
		CHECK( result=="www.gOOgle.com" );
		REQUIRE_NOTHROW( result=reverseshell::tools::sanitiseHostname("www.bbc.co.uk\n") );
		CHECK( result=="www.bbc.co.uk" );
	}
	WHEN( "Testing invalid hostnames" )
	{
		std::string result;
		REQUIRE_NOTHROW( result=reverseshell::tools::sanitiseHostname(" once upon a time") );
		CHECK( result=="" );
		REQUIRE_NOTHROW( result=reverseshell::tools::sanitiseHostname("../../") );
		CHECK( result=="" );
		REQUIRE_NOTHROW( result=reverseshell::tools::sanitiseHostname("ls /; cd /etc/") );
		CHECK( result=="ls" );
		REQUIRE_NOTHROW( result=reverseshell::tools::sanitiseHostname("www.google..com") );
		CHECK( result=="www.google" );
		REQUIRE_NOTHROW( result=reverseshell::tools::sanitiseHostname(".www.google.com") );
		CHECK( result=="" );
		REQUIRE_NOTHROW( result=reverseshell::tools::sanitiseHostname("www.google.com.") );
		CHECK( result=="www.google.com" );
		// Make sure the edge cases are okay
		REQUIRE_NOTHROW( result=reverseshell::tools::sanitiseHostname("Aa.Z-z09") );
		CHECK( result=="Aa.Z-z09" );
	}
} // end of 'SCENARIO ... reverseshell::tools::sanitiseHostname'
