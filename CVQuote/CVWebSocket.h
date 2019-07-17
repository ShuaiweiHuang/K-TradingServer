#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

using json = nlohmann::json;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

void CB_Message_Bitmex(websocketpp::connection_hdl, client::message_ptr msg);
void CB_Message_Binance(websocketpp::connection_hdl, client::message_ptr msg);

context_ptr CB_TLS_Init(const char * hostname, websocketpp::connection_hdl);
