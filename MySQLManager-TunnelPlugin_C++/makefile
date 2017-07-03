SOURCES = main.cpp ActiveTunnels.cpp BaseSocket.cpp HelperMethods.cpp INIParser.cpp JSONResponseGenerator.cpp \
LinuxSocket.cpp Logger.cpp LogRotation.cpp SocketException.cpp SocketListener.cpp SocketProcessor.cpp \
SSHTunnelForwarder.cpp StaticSettings.cpp StatusManager.cpp TunnelManager.cpp

boost_inc_path = /usr/include/boost
boost_lib_path = /usr/lib64/boost
rapidjson_inc_path = /usr/include/rapidjson
libssh2_lib_path = /usr/lib64/libssh2/lib64/
general_inc_path = /usr/include
openssl_inc_path = /usr/include/openssl


OBJECTS = $(SOURCES:.cpp=.o)
CC = g++
CFLAGS = -g -Iincludes -Wall -I$(openssl_inc_path) -I$(boost_inc_path)  -I$(general_inc_path) -I$(rapidjson_inc_path) -std=c++11
LDFLAGS = -L/usr/lib64/ -lcurl -L$(boost_lib_path) -lboost_system -lboost_filesystem -L$(libssh2_lib_path) -lssh2
EXENAME = MySQLManager



default:
	$(CC) $(CFLAGS) -o $(EXENAME) $(SOURCES) $(LDFLAGS)

clean:
	rm -fv *.o $(EXENAME)
