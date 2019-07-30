#ifndef CCVCLIENTSOCKET_H_
#define CCVCLIENTSOCKET_H_

#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

using json = nlohmann::json;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using namespace std;

enum TCVClientSocketStatus 
{
	cssNone,
	cssConnecting,
	cssConnected,
	cssDisconnect
};

class ICVClientSocketCallback;

class CCVClientSocket 
{
	private:

		ICVClientSocketCallback* m_pClientSocketCallback;

		int m_nSocket;

		TCVClientSocketStatus m_cssClientSocketStatus;

		struct addrinfo m_AddrInfo;
		struct addrinfo* m_AddrRes;
	protected:
		void Close();

	public:
		client m_cfd;
		client::connection_ptr m_conn;
		CCVClientSocket();
		CCVClientSocket(ICVClientSocketCallback* pClientSocketCallback);
		virtual ~CCVClientSocket();

		void Connect(string strHost, string strPort, string strName, int type);
		void Disconnect();

		int Send(const unsigned char* pBuf, int nSize);
		int Recv( unsigned char* pBuf, int nSize);

		int Recv();

		TCVClientSocketStatus GetStatus();
};
#endif /* CCVSOCKET_H_ */
