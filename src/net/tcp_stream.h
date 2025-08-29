#pragma once

#include <uv.h>

#include <string>

namespace svm {

class TcpStream {
 public:
  explicit TcpStream(uv_loop_t* loop, std::string url);
  ~TcpStream();

  void Connect() const;
  void Send();
  void Print() const;

  static void OnWrite(uv_write_t* req, int status);
  static void OnRead(uv_stream_t* stream, ssize_t size, const uv_buf_t* buf);
  static void OnConnect(uv_connect_t* req, int status);

 private:
  void SetStatus(int status);
  void SetData(const char* data);

  std::string url_;
  std::string data_;
  int status_;
};

}  // namespace svm
