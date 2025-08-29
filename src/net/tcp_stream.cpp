#include "tcp_stream.h"

namespace svm {

TcpStream::TcpStream(uv_loop_t* loop, std::string url) {}
TcpStream::~TcpStream() = default;

void TcpStream::Connect() const {}

void TcpStream::Send() {}

void TcpStream::Print() const {}

void TcpStream::OnWrite(uv_write_t* req, int status) {}

void TcpStream::OnRead(uv_stream_t* stream, ssize_t size, const uv_buf_t* buf) {
}

void TcpStream::OnConnect(uv_connect_t* req, int status) {}

void TcpStream::SetStatus(int status) {}

void TcpStream::SetData(const char* data) {}

}  // namespace svm
