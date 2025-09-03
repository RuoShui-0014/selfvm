#pragma once

#include <openssl/ssl.h>
#include <uv.h>

#include <string>

namespace svm {

class TcpStream {
 public:
  explicit TcpStream(uv_loop_t* loop);
  ~TcpStream();

  void DNSResolve();
  void DomainConnect(const std::string& domain, int port);
  void IPV4Connect(const char* ipv4);
  void Write(std::string data);
  void Read();
  void Close();

  static void OnDNSResolved(uv_getaddrinfo_t* get_addr_info,
                            int status,
                            addrinfo* addr_info);
  static void OnConnect(uv_connect_t* req, int status);
  static void OnWrite(uv_write_t* req, int status);
  static void OnRead(uv_stream_t* stream, ssize_t size, const uv_buf_t* buf);
  static void OnClose(uv_shutdown_t* req, int status);

 private:
  uv_loop_t* loop_;
  std::string host_domain_;
  std::string host_ipv4_;
  int port_;
  uv_getaddrinfo_t addr_info_{};
  uv_tcp_t tcp_{};
  uv_connect_t connect_{};
  uv_write_t write_{};
  uv_shutdown_t shutdown_{};

  SSL_CTX* ssl_ctx_{nullptr};
  SSL* ssl_{nullptr};
  BIO* bio_read_{nullptr};
  BIO* bio_write_{nullptr};
  uv_poll_t ssl_handshake_poll_;

  bool connected_{false};
};

}  // namespace svm
