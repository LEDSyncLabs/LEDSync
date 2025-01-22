#include "http_server.h"
#include "esp_http_server.h"
#include "utils.h"
#include "wifi_manager.h"
#include <esp_wifi.h>
#include <iostream>
#include <persistent_storage.h>

HttpServer::HttpServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  server = nullptr;
  httpd_start(&server, &config);

  httpd_uri_t root_uri = {.uri = "/",
                          .method = HTTP_GET,
                          .handler = root_handler,
                          .user_ctx = nullptr};
  httpd_register_uri_handler(server, &root_uri);

  httpd_uri_t submit_uri = {.uri = "/submit",
                            .method = HTTP_POST,
                            .handler = submit_handler,
                            .user_ctx = nullptr};
  httpd_register_uri_handler(server, &submit_uri);

  httpd_uri_t submit_ap_uri = {.uri = "/submit_ap",
                               .method = HTTP_POST,
                               .handler = submit_handler_ap,
                               .user_ctx = nullptr};
  httpd_register_uri_handler(server, &submit_ap_uri);

  std::cout << "HTTP server started" << std::endl;
}

void HttpServer::stop() { httpd_stop(server); }

esp_err_t HttpServer::root_handler(httpd_req_t *req) {
  std::string ssid, password, ap_ssid;

  WifiManager::STA::load_credentials(ssid, password);
  WifiManager::AP::load_ssid(ap_ssid);

  PersistentStorage storage("MQTT");

  std::string deviceName = storage.getValue("deviceName");
  std::string secretKey = storage.getValue("secretKey");

  std::string response =  R"rawliteral(
        <html>
        <head>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    background-color: #f4f4f4;
                    margin: 0;
                    padding: 20px;
                }
                h1 {
                    color: #333;
                    border-bottom: 2px solid #007BFF;
                    padding-bottom: 5px;
                }
                form {
                    background: #fff;
                    padding: 20px;
                    border-radius: 8px;
                    box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
                    margin-bottom: 20px;
                }
                input[type="text"] {
                    width: 100%;
                    padding: 10px;
                    margin: 10px 0;
                    border: 1px solid #ccc;
                    border-radius: 4px;
                    box-sizing: border-box;
                }
                input[type="submit"] {
                    background-color: #007BFF;
                    color: white;
                    border: none;
                    padding: 10px 20px;
                    cursor: pointer;
                    border-radius: 4px;
                    font-size: 16px;
                }
                input[type="submit"]:hover {
                    background-color: #0056b3;
                }
            </style>
        </head>
        <body>
            <h1>WiFi Configuration</h1>
            <form action="/submit" method="post">
                SSID: <input type="text" name="ssid" value="{1}"><br>
                Password: <input type="text" name="password" value="{2}"><br>
                <input type="submit" value="Submit">
            </form>

            <h1>Access Point Configuration</h1>
            <form action="/submit_ap" method="post">
                SSID: <input type="text" name="ssid" value="{3}"><br>
                <input type="submit" value="Submit">
            </form>

            <h1>Device Info</h1>

            {4}         
        </body>
        </html>
    )rawliteral";

  Utils::replace(response, "{1}", ssid);
  Utils::replace(response, "{2}", password);
  Utils::replace(response, "{3}", ap_ssid);

  if(deviceName != "" && secretKey != "") {

     std::string deviceForm = R"rawliteral(
      <h2>Go to <a href="https://ledsync.spookyless.net">ledsync.spookyless.net</a> to add your device</h2>
      
      <form>
        Device Name: <input type="text" value="{1}" readonly><br>
        Secret Key: <input type="text" value="{2}" readonly><br>
      </form>            
    )rawliteral";

    Utils::replace(deviceForm, "{1}", deviceName);
    Utils::replace(deviceForm, "{2}", secretKey);
    Utils::replace(response, "{4}", deviceForm);

  }
  else{
    Utils::replace(response, "{4}", "<h2 style='color:red;'>Connect to Internet first to be able to register the device</h2>");
  }
  Utils::replace(response, "{4}", deviceName);

  httpd_resp_send(req, response.c_str(), HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

esp_err_t HttpServer::submit_handler(httpd_req_t *req) {
  char buf[100];
  int len = httpd_req_recv(req, buf, sizeof(buf));

  if (len <= 0) {
    return ESP_FAIL;
  }

  buf[len] = '\0';
  std::string body(buf);

  size_t ssid_start = body.find("ssid=") + 5;
  size_t ssid_end = body.find("&", ssid_start);
  size_t password_start = body.find("password=") + 9;

  std::string ssid =
      Utils::url_decode(body.substr(ssid_start, ssid_end - ssid_start));
  std::string password = Utils::url_decode(body.substr(password_start));

  WifiManager::STA::save_credentials(ssid, password);

  httpd_resp_send(req, "Credentials set. Restart device.",
                  HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

esp_err_t HttpServer::submit_handler_ap(httpd_req_t *req) {
  char buf[100];
  int len = httpd_req_recv(req, buf, sizeof(buf));

  if (len <= 0) {
    return ESP_FAIL;
  }

  buf[len] = '\0';
  std::string body(buf);

  size_t ssid_start = body.find("ssid=") + 5;

  std::string ssid = Utils::url_decode(body.substr(ssid_start));

  WifiManager::AP::save_ssid(ssid);

  httpd_resp_send(req, "AP SSID set. Restart device.", HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}
