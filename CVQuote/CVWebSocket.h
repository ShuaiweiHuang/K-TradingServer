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

void on_message_bitmex(websocketpp::connection_hdl, client::message_ptr msg);
void on_message_binance(websocketpp::connection_hdl, client::message_ptr msg);

bool verify_subject_alternative_name(const char * hostname, X509 * cert);
bool verify_common_name(const char * hostname, X509 * cert);
bool verify_certificate(const char * hostname, bool preverified, boost::asio::ssl::verify_context& ctx);

int make_socket (uint16_t port);


context_ptr on_tls_init(const char * hostname, websocketpp::connection_hdl);
