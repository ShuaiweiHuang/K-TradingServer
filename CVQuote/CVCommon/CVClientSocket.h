#ifndef CSKCLIENTSOCKET_H_
#define CSKCLIENTSOCKET_H_

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
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
using namespace std;

enum TSKClientSocketStatus 
{
	cssNone,
	cssConnecting,
	cssConnected,
	cssDisconnect
};

class ISKClientSocketCallback;

class CSKClientSocket 
{
	private:

		ISKClientSocketCallback* m_pClientSocketCallback;

		int m_nSocket;

		TSKClientSocketStatus m_cssClientSocketStatus;

		struct addrinfo m_AddrInfo;
		struct addrinfo* m_AddrRes;
		static context_ptr CB_TLS_Init(const char *, websocketpp::connection_hdl);
		static void CB_Message_Binance(websocketpp::connection_hdl, client::message_ptr msg);
		static void CB_Message_Bitmex (websocketpp::connection_hdl, client::message_ptr msg);

	protected:
		void Close();

	public:
		client m_cfd;
		CSKClientSocket();
		CSKClientSocket(ISKClientSocketCallback* pClientSocketCallback);
		virtual ~CSKClientSocket();

		void Connect(string strHost, string strPort, string strName, int type);
		void Disconnect();

		int Send(const unsigned char* pBuf, int nSize);
		int Recv( unsigned char* pBuf, int nSize);

		int Recv();

		TSKClientSocketStatus GetStatus();
};
#endif /* CSKSOCKET_H_ */
