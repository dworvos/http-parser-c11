
#include <iostream>
#include <string>
#include <http_parser.hpp>

const char* test = "GET /favicon.ico HTTP/1.1\r\n"
"Host: 0.0.0.0=5000\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
"Accept-Language: en-us,en;q=0.5\r\n"
"Accept-Encoding: gzip,deflate\r\n"
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
"Keep-Alive: 300\r\n"
"Connection: keep-alive\r\n"
"\r\n";

const char* bad = "GET /favicon.ico HTTP/1.1\r\n"
"absdfkajl\r\n";

const char* url1 = "GET /favi";
const char* url2 = "con.ico HTTP/1.1\r\n\n";

const char* chunked = "POST /post_chunked_all_your_base HTTP/1.1\r\n"
"Transfer-Encoding: chunked\r\n"
"\r\n"
"1e\r\nall your base are belong to us\r\n"
"d\r\nanother chunk\r\n"
"0\r\n"
"\r\n";

struct Nil 
{ 
  template< typename T > int operator()(const T&) 
  {
    std::cout << typeid(T).name() << std::endl;
    return 0; 
  } 
};

struct RequestHandler
{ 
  int operator()(const nodejs::events::on_message_begin&) 
  {
    return 0;
  }

  int operator()(const nodejs::events::on_message_end&) 
  {
    return 0;
  }

  int operator()(const nodejs::events::on_header_begin&)
  {
    return 0; 
  } 
 
  int operator()(const nodejs::events::on_header_end& a) 
  {
    std::cout << "url: " << a.url << "(" << a.method << ")" << std::endl;
    return 0; 
  } 
  
  int operator()(const nodejs::events::on_header& h) 
  {
    std::cout << h.key << ": " << h.value << std::endl;
    return 0; 
  } 

  int operator()(const nodejs::events::on_body& h) 
  {
    std::cout << "body: " << h.body << std::endl;
    return 0; 
  } 


};

int main(int argc, char** argv)
{
  try
  {
    nodejs::http_request_parser<Nil> parser;
    parser.parse(bad, strlen(bad));
  }
  catch(const nodejs::exceptions::parse_error&)
  {
    std::cerr << "failed to parse" << std::endl;
  }

  std::cout << "===" << std::endl;

  try
  {
    nodejs::http_request_parser<RequestHandler> parser;
    parser.parse(test, strlen(test));
  }
  catch(const nodejs::exceptions::parse_error&)
  {
    std::cerr << "failed to parse" << std::endl;
  }

  std::cout << "===" << std::endl;

  try
  {
    nodejs::http_request_parser<RequestHandler> parser;
    parser.parse(url1, strlen(url1));
    parser.parse(url2, strlen(url2));
  }
  catch(const nodejs::exceptions::parse_error&)
  {
    std::cerr << "failed to parse" << std::endl;
  }

  std::cout << "===" << std::endl;

  try
  {
    nodejs::http_request_parser<RequestHandler> parser;
    parser.parse(chunked, strlen(chunked));
  }
  catch(const nodejs::exceptions::parse_error&)
  {
    std::cerr << "failed to parse" << std::endl;
  }


  //std::cout << sizeof(nodejs::http_request_parser<int>) << std::endl;
  return 0;
}
