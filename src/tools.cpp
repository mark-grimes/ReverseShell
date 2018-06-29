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

std::string reverseshell::tools::sanitiseHostname( const std::string& hostname )
{
	size_t index;
	bool needNonDot=true; // Can't have a dot immedately after another one
	for( index=0; index<hostname.size(); ++index )
	{
		if( (hostname[index]<'0' || hostname[index]>'9')
			&& (hostname[index]<'A' || hostname[index]>'Z')
			&& (hostname[index]<'a' || hostname[index]>'z')
			&& hostname[index]!='-' && hostname[index]!='.' ) break;
		// Additional checks so that dots aren't together
		if( hostname[index]=='.' )
		{
			if( needNonDot ) break;
			needNonDot=true;
		}
		else needNonDot=false;
	}

	// Make sure it doesn't finish on a dot
	if( index!=0 )
	{
		if( hostname[index-1]=='.' ) --index;
	}

	return hostname.substr(0,index);
}
