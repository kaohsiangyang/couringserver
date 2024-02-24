#include "http_server.hpp"

int main() {
  couringserver::http_server http_server;
  http_server.listen("8080");
}
