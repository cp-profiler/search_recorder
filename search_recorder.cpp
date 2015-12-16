#include <string>
#include <iostream>
#include <cstdio>
#include <iomanip>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif

#include <fstream>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>

#include "message.pb.hh"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

using boost::asio::ip::tcp;
using google::protobuf::io::OstreamOutputStream;

bool writeDelimitedTo(
    const google::protobuf::MessageLite& message,
    google::protobuf::io::ZeroCopyOutputStream* rawOutput) {
  // We create a new coded stream for each message.  Don't worry, this is fast.
  google::protobuf::io::CodedOutputStream output(rawOutput);

  // Write the size.
  const int size = message.ByteSize();
  output.WriteVarint32(size);

  uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
  if (buffer != NULL) {
    // Optimization:  The message fits in one buffer, so use the faster
    // direct-to-array serialization path.
    message.SerializeWithCachedSizesToArray(buffer);
  } else {
    // Slightly-slower path when the message is multiple buffers.
    message.SerializeWithCachedSizes(&output);
    if (output.HadError()) return false;
  }

  return true;
}

class tcp_connection : public boost::enable_shared_from_this<tcp_connection> {
public:
    typedef boost::shared_ptr<tcp_connection> pointer;

    static pointer create(OstreamOutputStream& raw_output, boost::asio::io_service& io_service) {
        return pointer(new tcp_connection(raw_output, io_service));
    }

    tcp::socket& socket() {
        return socket_;
    }

    void start() {
        boost::asio::async_read(socket_,
                                boost::asio::buffer(buffer, 4),
                                boost::bind(&tcp_connection::handle_read_header, shared_from_this(),
                                            boost::asio::placeholders::error));
    }

    void handle_read_header(const boost::system::error_code& error) {
        if (error) {
            std::cerr << "error reading header\n";
            return;
        }
        uint32_t* int32p = reinterpret_cast<uint32_t*>(buffer);
        len = *int32p;
        if (len > 10000) abort();
        boost::asio::async_read(socket_,
                                boost::asio::buffer(buffer, len),
                                boost::bind(&tcp_connection::handle_read_body, shared_from_this(),
                                            boost::asio::placeholders::error));
    }

    void handle_read_body(const boost::system::error_code& error) {
        if (error) return;
        message::Node node;
        node.ParseFromArray(buffer, len);
        writeDelimitedTo(node, &raw_output_);
        if (node.type() == message::Node::DONE)
            socket_.get_io_service().stop();
        else
            start();
    }

private:
    tcp_connection(OstreamOutputStream& raw_output, boost::asio::io_service& io_service)
        : socket_(io_service), raw_output_(raw_output)
    { }

    tcp::socket socket_;
    char buffer[10000];
    int len;
    OstreamOutputStream& raw_output_;
};


class tcp_server {
public:
    tcp_server(OstreamOutputStream& raw_output, boost::asio::io_service& io_service)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), 6565)),
          raw_output_(raw_output) {
        start_accept();
    }

private:
    void start_accept() {
        tcp_connection::pointer new_connection =
            tcp_connection::create(raw_output_, acceptor_.get_io_service());

        acceptor_.async_accept(new_connection->socket(),
                               boost::bind(&tcp_server::handle_accept, this,
                                           new_connection,
                                           boost::asio::placeholders::error));
    }

    void handle_accept(tcp_connection::pointer new_connection,
                       const boost::system::error_code& error) {
        if (!error) new_connection->start();
        start_accept();
    }

    tcp::acceptor acceptor_;
    OstreamOutputStream& raw_output_;
};


        
int main(int argc, char** argv) {

  std::string path = "data.db";

  if (argc == 1) {
    std::cout << "File location: " << path << std::endl;
  } else if (argc == 2) {
    path = argv[1];
    std::cout << "File location: " << path << std::endl;
  } else if (argc > 2) {
    std::cerr << "Too many arguments. Usage: search_recorder [FILE]" << std::endl;
    return 1;
  }

  std::ofstream outputFile(path, std::ios::out | std::ios::binary);
  OstreamOutputStream raw_output(&outputFile);

  try {
      boost::asio::io_service io_service;
      tcp_server server(raw_output, io_service);
      io_service.run();
  } catch (std::exception& e) {
      std::cerr << e.what() << "\n";
  }

  return 0;
}
