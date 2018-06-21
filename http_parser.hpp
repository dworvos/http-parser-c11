// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE. 

#ifndef CPP11_HTTP_PARSER_HPP
#define CPP11_HTTP_PARSER_HPP

#include <utility>
#include <string>

namespace nodejs {

namespace c {
#include <http-parser/http_parser.c>
}

namespace detail {

  template< typename T >
  int on_message_begin(c::http_parser* parser) 
  {
    return reinterpret_cast<T*>(parser->data)->on_message_begin();
  }

  template< typename T >
  int on_headers_complete(c::http_parser* parser) 
  {
    return reinterpret_cast<T*>(parser->data)->on_headers_complete();
  }

  template< typename T >
  int on_message_complete(c::http_parser* parser)
  {
    return reinterpret_cast<T*>(parser->data)->on_message_complete();
  }

  template< typename T >
  int on_chunk_header(c::http_parser* parser)
  {
    return reinterpret_cast<T*>(parser->data)->on_chunk_header();
  }

  template< typename T >
  int on_chunk_complete(c::http_parser* parser)
  {
    return reinterpret_cast<T*>(parser->data)->on_chunk_complete();
  }

  template< typename T >
  int on_url(c::http_parser* parser, const char* at, size_t length)
  {
    return reinterpret_cast<T*>(parser->data)->on_url(at, length);
  }

  template< typename T >
  int on_header_field(c::http_parser* parser, const char* at, size_t length)
  {
    return reinterpret_cast<T*>(parser->data)->on_header_field(at, length);
  }

  template< typename T >
  int on_status(c::http_parser* parser, const char* at, size_t length)
  {
    return reinterpret_cast<T*>(parser->data)->on_status(at, length);
  }

  template< typename T >
  int on_body(c::http_parser* parser, const char* at, size_t length)
  {
    return reinterpret_cast<T*>(parser->data)->on_body(at, length);
  }

  template< typename T >
  int on_header_value(c::http_parser* parser, const char* at, size_t length)
  {
    return reinterpret_cast<T*>(parser->data)->on_header_value(at, length);
  }

}

namespace exceptions {

  class parse_error : public std::runtime_error
  {
  public:

    parse_error() : std::runtime_error("failed to parse request") {}

  };

  class upgrade_requested : public std::runtime_error
  {
  public:

    upgrade_requested() : std::runtime_error("upgrade requested") {}

  };

}

enum http_method {
  GET = c::HTTP_GET,
  HEAD = c::HTTP_HEAD,
  POST = c::HTTP_POST,
  PUT = c::HTTP_PUT,
  DELETE = c::HTTP_DELETE,
  CONNECT = c::HTTP_CONNECT,
  OPTIONS = c::HTTP_OPTIONS,
  TRACE = c::HTTP_TRACE,
};

namespace events {

  struct on_message_begin {};
  struct on_message_end {};
  struct on_header { std::string key; std::string value;};
  struct on_header_begin {};
  struct on_header_end { std::string url; http_method method; int version; };
  struct on_body { std::string body; };

}

template< typename Data >
class http_request_parser
{

  enum header_state {
    none,
    field,
    value
  };

public:

  typedef http_request_parser<Data> parser_type;

  friend int detail::on_message_begin<parser_type>(c::http_parser*);
  friend int detail::on_headers_complete<parser_type>(c::http_parser*);
  friend int detail::on_message_complete<parser_type>(c::http_parser*);
  friend int detail::on_chunk_header<parser_type>(c::http_parser*);
  friend int detail::on_chunk_complete<parser_type>(c::http_parser*);
  friend int detail::on_url<parser_type>(c::http_parser*, const char* at, size_t length);
  friend int detail::on_header_field<parser_type>(c::http_parser*, const char* at, size_t length);
  friend int detail::on_body<parser_type>(c::http_parser*, const char* at, size_t length);
  friend int detail::on_header_value<parser_type>(c::http_parser*, const char* at, size_t length);

  http_request_parser() :
    parser(new c::http_parser)
  {
    current_url.reserve(256);
    current_field.reserve(64);
    current_value.reserve(64);
    current_body.reserve(4096);

    reset();

    c::http_parser_settings_init(&settings);
    settings.on_message_begin = detail::on_message_begin<parser_type>;
    settings.on_headers_complete = detail::on_headers_complete<parser_type>;
    settings.on_message_complete = detail::on_message_complete<parser_type>;
    settings.on_chunk_header = detail::on_chunk_header<parser_type>;
    settings.on_chunk_complete = detail::on_chunk_complete<parser_type>;
    settings.on_header_field = detail::on_header_field<parser_type>;
    settings.on_body = detail::on_body<parser_type>;
    settings.on_header_value = detail::on_header_value<parser_type>;
    settings.on_url = detail::on_url<parser_type>;
    //settings.on_status for response parsing?
  }

  template< typename... Args >
  http_request_parser(Args&&... args) :
    http_request_parser(),
    Data(std::forward<Args>(args)...)
  {
  }

  void parse(const char* buffer, ssize_t length)
  {
    // note we pass length==0 to signal that EOF has been received.
    size_t nparsed = c::http_parser_execute(parser, &settings, buffer, length);

    if(parser->upgrade) 
    {
      throw exceptions::upgrade_requested();
    }
    else if(nparsed != length)
    {
      reset();
      throw exceptions::parse_error();
    }

  }

  void reset()
  {
    c::http_parser_init(parser, c::HTTP_REQUEST);
    parser->data = this;

    current_url.clear();
    current_field.clear();
    current_value.clear();
    current_body.clear();

    last_header_state = header_state::none;
  }

  size_t set_max_url_size(size_t new_size) { size_t old_size = max_url_size; max_url_size = new_size; return old_size; }
  size_t get_max_url_size() const { return max_url_size; }

  size_t set_max_field_size(size_t new_size) { size_t old_size = max_field_size; max_field_size = new_size; return old_size; }
  size_t get_max_field_size() const { return max_field_size; }

protected:

  size_t max_url_size = 1024 * 2;
  size_t max_field_size = 1024 * 4;
  size_t max_body_size = 1024 * 1024;

  Data data;
  c::http_parser* parser;
  std::string current_url;
  std::string current_field;
  std::string current_value;
  std::string current_body;
  header_state last_header_state;
  c::http_parser_settings settings;

  int on_message_begin() 
  {
    return data(events::on_message_begin());
  }

  int on_url(const char* at, size_t length)
  {
    current_url.append(at, length);
    return current_url.size() > max_url_size;
  }

  int on_headers_complete() 
  {
    return data(events::on_header_end { current_url, (http_method)parser->method, parser->http_major << 16 | parser->http_minor });
  }

  int on_message_complete()
  {
    return data(events::on_message_end());
  }

  int on_chunk_header()
  {
    current_body.clear();
    return parser->content_length > max_body_size;
  }

  int on_chunk_complete()
  {
    if(current_body.size() > 0)
    {
      return data(events::on_body { current_body });
    }
    return 0;
  }

  int on_body(const char* at, size_t length)
  {
    current_body.append(at, length);
    return 0;
  }

  int on_header_field(const char* at, size_t length)
  {
    if(last_header_state == header_state::value)
    {
      data(events::on_header { current_field, current_value });

      current_field.clear();
      current_value.clear();
    }
    else if(last_header_state == header_state::none)
    {
      data(events::on_header_begin());
    }

    last_header_state = header_state::field;

    current_field.append(at, length);
    return current_field.size() > max_field_size;
  }

  int on_header_value(const char* at, size_t length)
  {
    last_header_state = header_state::value;

    current_value.append(at, length);
    return current_value.size() > max_field_size;
  }

};

}

#endif
