#include "http_server.hpp"

int main() {
  co_uring_http::http_server http_server;
  http_server.listen("8080");
}
