/**
	Throws an exception in the event that socket error has occurred
*/

#include "SocketException.h"

SocketException::SocketException(char const* const message) throw()
	: std::runtime_error(message)
{

}

char const * SocketException::what() const throw()
{
	return exception::what();
}