#include "tcp_stream.h"

#include <iostream>
#include <ostream>

#include "base/config.h"

namespace svm {

TcpStream::TcpStream(uv_loop_t* loop) : loop_{loop} {
  addr_info_.data = this;
  tcp_.data = this;
  connect_.data = this;
  write_.data = this;
  shutdown_.data = this;
}
TcpStream::~TcpStream() {
  if (connected_) {
    Close();
  }
}

void TcpStream::DNSResolve() {
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  uv_getaddrinfo(uv_default_loop(), &addr_info_, OnDNSResolved,
                 host_domain_.c_str(), nullptr, &hints);
}

void TcpStream::DomainConnect(const std::string& domain, int port) {
  host_domain_ = domain;
  port_ = port;
  DNSResolve();
}

void TcpStream::IPV4Connect(const char* ipv4) {
  host_ipv4_ = ipv4;

  uv_tcp_init(uv_default_loop(), &tcp_);
  stream_ = reinterpret_cast<uv_stream_t*>(&tcp_);

  sockaddr_in dest{};
  uv_ip4_addr(ipv4, port_, &dest);
  uv_tcp_connect(&connect_, &tcp_, reinterpret_cast<const sockaddr*>(&dest),
                 OnConnect);
}

void TcpStream::Write() {}

void TcpStream::Read() {}

void TcpStream::Close() {}

void TcpStream::OnDNSResolved(uv_getaddrinfo_t* get_addr_info,
                              int status,
                              addrinfo* addr_info) {
  if (!status) {
    char addr[16]{};
    uv_ip4_name(reinterpret_cast<sockaddr_in*>(addr_info->ai_addr), addr, 16);
    uv_freeaddrinfo(addr_info);

    auto* tcp_stream{static_cast<TcpStream*>(get_addr_info->data)};
    tcp_stream->IPV4Connect(addr);
#ifdef DEBUG_FLAG
    std::cout << addr << std::endl;
#endif
  }
}

void TcpStream::OnConnect(uv_connect_t* req, int status) {
  if (!status) {
    auto* tcp_stream{static_cast<TcpStream*>(req->data)};
    tcp_stream->connected_ = true;
  }
}

void TcpStream::OnWrite(uv_write_t* req, int status) {}

void TcpStream::OnRead(uv_stream_t* stream, ssize_t size, const uv_buf_t* buf) {
}

void TcpStream::OnClose(uv_shutdown_t* req, int status) {}

}  // namespace svm
