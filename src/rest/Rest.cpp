#include "rest/Rest.hpp"
#include "common/logger.hpp"

namespace neuro {
namespace rest {

Rest::Rest(const std::string &endpoint, const Port port) : _server(O_POOL) {
  _server.setHostname(endpoint);
  _server.setPort(port);
  _root = std::make_unique<Onion::Url>(&_server);
}

void Rest::run_async() {
  _thread = std::thread([this]() { _server.listen(); });
}

void Rest::add_endpoint(const std::string &endpoint, Rest::Handler &&handler) {
  _root->add(endpoint, std::move(handler));
}

void Rest::add_endpoint(const std::string &endpoint,
                        const std::string &file_path) {
  _root->add(endpoint, file_path);
}

void Rest::stop() { _server.listenStop(); }

Rest::~Rest() {
  LOG_INFO << "Stopping rest service";
  _thread.join();
  LOG_DEBUG << "rest service stopped";
}

}  // namespace rest
}  // namespace neuro
