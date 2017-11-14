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

int main( int argc, char* argv[] )
{
	const redi::pstreams::pmode mode = redi::pstreams::pstdout|redi::pstreams::pstderr;
	redi::pstream child("bash", mode);
	char buf[1024];
	std::streamsize n;
	bool finished[2] = { false, false };

	while (!finished[0] || !finished[1])
	{
		if (!finished[0])
		{
			while ((n = child.err().readsome(buf, sizeof(buf))) > 0) std::cerr.write(buf, n);
			if (child.eof())
			{
				finished[0] = true;
				if (!finished[1]) child.clear();
			}
		}

		if (!finished[1])
		{
			while ((n = child.out().readsome(buf, sizeof(buf))) > 0) std::cout.write(buf, n).flush();
			if (child.eof())
			{
				finished[1] = true;
				if (!finished[0]) child.clear();
			}
		}
	}

	return 0;
}
