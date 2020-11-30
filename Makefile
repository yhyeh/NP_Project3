CXX=g++
CXXFLAGS=-std=c++14 -Wall -pedantic -pthread -lboost_system
CXX_INCLUDE_DIRS=/usr/local/include
CXX_INCLUDE_PARAMS=$(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS=/usr/local/lib
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))

part2: cgi_server.cpp
	g++ cgi_server.cpp -o cgi_server -lws2_32 -lwsock32 -std=c++14

part1: http_server.cpp console.cgi
	$(CXX) http_server.cpp -o http_server $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

console.cgi: console.cpp
	$(CXX) console.cpp -o console.cgi $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

clean:
	rm -f http_server console.cgi cgi_server
clean_part1:
	rm -f http_server console.cgi 
clean_part2:
	rm -f cgi_server