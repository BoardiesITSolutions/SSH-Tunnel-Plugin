#ifndef SOCKETEXCEPTION_H
#define SOCKETEXCEPTION_H

#include <iostream>
#include <stdexcept>
#include <exception>
using namespace std;

class SocketException : public runtime_error
{
public:
	SocketException(char const* const message) throw();
	virtual char const* what() const throw();
};

#endif //!SOCKETEXCEPTION_H