
test.exe: http_parser.cpp http_parser.hpp
		g++ -O3 -std=c++11 http_parser.cpp -I. -o $@

