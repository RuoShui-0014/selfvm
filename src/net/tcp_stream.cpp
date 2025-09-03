#include "tcp_stream.h"

#include <iostream>
#include <ostream>
#include <sstream>

#include "base/config.h"

namespace svm {

TcpStream::TcpStream(uv_loop_t* loop) : loop_{loop} {
  addr_info_.data = this;
  tcp_.data = this;
  connect_.data = this;
  write_.data = this;
  shutdown_.data = this;

  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
  ssl_ctx_ = SSL_CTX_new(SSLv23_client_method());
  if (!ssl_ctx_) {
    std::cerr << "SSL_CTX_new failed" << std::endl;
  }

  SSL_CTX_set_options(ssl_ctx_, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
  // SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_PEER, VerifyCertificate);
  SSL_CTX_set_verify_depth(ssl_ctx_, 4);

  ssl_ = SSL_new(ssl_ctx_);
  if (!ssl_) {
    std::cerr << "SSL failed" << std::endl;
  }

  bio_read_ = BIO_new(BIO_s_mem());
  bio_write_ = BIO_new(BIO_s_mem());
  SSL_set_bio(ssl_, bio_read_, bio_write_);
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

  sockaddr_in dest{};
  uv_ip4_addr(ipv4, port_, &dest);
  uv_tcp_connect(&connect_, &tcp_, reinterpret_cast<const sockaddr*>(&dest),
                 OnConnect);
}

void TcpStream::Write(std::string data) {
  int ret{SSL_connect(ssl_)};
  if (ret == 1) {
    std::cout << "SSL handshake completed successfully" << std::endl;
  } else {
    int err = SSL_get_error(ssl_, ret);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
    } else {
    }
  }

  std::stringstream request;
  request << "GET / HTTP/1.1\r\n";
  request << "Host: " << host_domain_ << "\r\n";
  request << "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
             "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/135.0.0.0 "
             "Safari/537.36\r\n";
  request << "Accept: "
             "text/html,application/xhtml+xml,application/xml;q=0.9,image/"
             "avif,image/webp,image/apng,*/*;q=0.8,application/"
             "signed-exchange;v=b3;q=0.7\r\n\r\n";
  request << "Connection: close\r\n\r\n";
  std::string request_str = request.str();
  uv_buf_t buf = uv_buf_init(new char[request_str.size()], request_str.size());
  memcpy(buf.base, request_str.c_str(), request_str.size());
  write_.data = this;

  uv_write(&write_, reinterpret_cast<uv_stream_t*>(&tcp_), &buf, 1, OnWrite);
}

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
    tcp_stream->Write("Hello World!\r\n");
  }
}

void OnAlloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  buf->base = new char[suggested_size]{};
  buf->len = suggested_size;
}
void TcpStream::OnWrite(uv_write_t* req, int status) {
  if (!status) {
    auto* tcp_stream{static_cast<TcpStream*>(req->data)};
    uv_read_start(reinterpret_cast<uv_stream_t*>(&tcp_stream->tcp_), OnAlloc,
                  OnRead);
  }
}

void TcpStream::OnRead(uv_stream_t* stream, ssize_t size, const uv_buf_t* buf) {
  if (size) {
    uv_read_stop(stream);
    delete[] buf->base;
  }
}

void TcpStream::OnClose(uv_shutdown_t* req, int status) {}

}  // namespace svm
