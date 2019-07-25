#ifndef CSKCLIENTSOCKET_H_
#define CSKCLIENTSOCKET_H_

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
	protected:
		void Close();

	public:
		client m_cfd;
		client::connection_ptr m_conn;
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
