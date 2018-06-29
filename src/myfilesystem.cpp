#include "myfilesystem.h"
#include <limits.h>

std::string myfilesystem::canonical( const char* path, std::error_code& error )
{
	char actualPath[PATH_MAX+1];
	char* pResult=realpath( path, actualPath );
	if( pResult==nullptr )
	{
		error=std::error_code( errno, std::generic_category() );
		return std::string();
	}
	else return pResult;
}

std::string myfilesystem::canonical( const char* path )
{
	std::error_code error;
	auto returnValue=canonical( path, error );
	if( error ) throw std::system_error( error, path );
	return returnValue;
}
