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
