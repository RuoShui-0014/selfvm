//
// Created by ruoshui on 2025/8/8.
//

#include "request.h"

#include <iostream>
#include <ostream>

namespace svm {

Request::Request(uv_loop_t* loop, std::string url)
    : url_{std::move(url)}, status_{0} {}
Request::~Request() = default;

void Request::Connect() const {}

void Request::Send() {}

void Request::Print() const {
  std::cout << data_ << std::endl;
}

void Request::OnWrite(uv_write_t* writer, int status) {}
void Request::OnRead(uv_stream_t* stream, ssize_t size, const uv_buf_t* buf) {}
void Request::OnConnect(uv_connect_t* connect, int status) {}

void Request::SetStatus(int status) {
  status_ = status;
}
void Request::SetData(const char* data) {
  data_ = data;
}

}  // namespace svm
