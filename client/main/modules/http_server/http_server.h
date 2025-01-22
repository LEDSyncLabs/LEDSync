#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <esp_err.h>
#include <string>
#include <esp_http_server.h>


class HttpServer {
public:
  httpd_handle_t server;

  HttpServer();
  // ~HttpServer();
  void stop();

private:
  // Add any private members or methods if needed
  static esp_err_t root_handler(httpd_req_t *req);
  static esp_err_t submit_handler(httpd_req_t *req);
  static esp_err_t submit_handler_ap(httpd_req_t *req);
};

#endif // HTTP_SERVER_H