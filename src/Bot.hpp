#ifndef NEURO_SRC_BOT_HPP
#define NEURO_SRC_BOT_HPP

#include <memory>
#include "messages/Subscriber.hpp"
#include "crypto/Ecc.hpp"

namespace neuro {

class Bot {
private:
  messages::Subscriber _subscriber;
  messages::config::Config _config;
  std::shared_ptr<crypto::Ecc> _keys;
  
public:
  Bot(const std::string &configuration_path) {
    messages::from_json_file(configuration_path, &_config);


    subscriber.subscribe(std::type_index(typeid(messages::Hello)),
			 [this](const messages::Header &header,
			    const messages::Body &body) {
			   this->handle_hello(header, body);
			 });
    
  }


					 
  
  virtual ~Bot() {
    //save_config(_config);
  }
};

}  // neuro

#endif /* NEURO_SRC_BOT_HPP */
