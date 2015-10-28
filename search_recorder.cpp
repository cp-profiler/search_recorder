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

#include "third-party/zmq.hpp"
#include "message.pb.hh"

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


int main(int argc, char** argv) {

  using google::protobuf::io::OstreamOutputStream;

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

  zmq::context_t context(1);
  zmq::socket_t receiver (context, ZMQ_PULL);

  receiver.bind("tcp://*:6565");
  std::cout << "listening to port: 6565\n";

    if (!outputFile) {
      std::cerr << "Can't open the file" << std::endl;
      return 1;
    }

    while (true) {
      
      zmq::message_t msg;
      
      receiver.recv(&msg);

      message::Node node;
      node.ParseFromArray(msg.data(), msg.size());
      writeDelimitedTo(node, &raw_output);

      if (node.type() == message::Node::DONE)
        break;

    }

  return 0;
}