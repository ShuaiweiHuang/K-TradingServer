#include "CVWebSocket.h"
#include "CVServiceHandler.h"
#include "CVServer.h"
#include "CVGlobal.h"

using namespace std;

void CB_Message_Binance(websocketpp::connection_hdl, client::message_ptr msg) {
#ifdef DEBUG
	printf("[on_message_binance]\n");
#endif
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];

	string str = msg->get_payload();
	string price_str, size_str, side_str, time_str, symbol_str;
	json jtable = json::parse(str.c_str());

	static CSKClients* pClients = CSKClients::GetInstance();
	if(pClients == NULL)
		throw "GET_CLIENTS_ERROR";

	memset(netmsg, 0, BUFFERSIZE);
	memset(timemsg, 0, 8);

	static int tick_count_binance=0;
	time_str   = "00000000";
	symbol_str = to_string(jtable["s"]);
	symbol_str.erase(remove(symbol_str.begin(), symbol_str.end(), '\"'), symbol_str.end()); 
	price_str  = to_string(jtable["p"]);
	price_str.erase(remove(price_str.begin(), price_str.end(), '\"'), price_str.end()); 
	size_str   = to_string(jtable["q"]);
	size_str.erase(remove(size_str.begin(), size_str.end(), '\"'), size_str.end()); 
	int size_int = stof(size_str) * SCALE_VOL_BINANCE;
	size_str = to_string(size_int);

	sprintf(netmsg, "01_ID=%s.BINANCE,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,",
		symbol_str.c_str(), time_str.c_str(), price_str.c_str(), size_str.c_str(), tick_count_binance++, pClients->m_strEPIDNum.c_str());

	int msglen = strlen(netmsg);
	netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
	netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;

#if DEBUG
		cout << setw(4) << jtable << endl;
		cout << netmsg << endl;
#endif
	vector<shared_ptr<CSKClient> >::iterator iter = pClients->m_vClient.begin();

	while(iter != pClients->m_vClient.end())
	{
		CSKClient* pClient = (*iter).get();
		if(pClient->GetStatus() == csOffline && (*iter).unique()) {
			iter++;
			continue;
		}
		pClient->SendAll(NULL, netmsg, strlen(netmsg));
		iter++;
	}
}


void CB_Message_Bitmex(websocketpp::connection_hdl, client::message_ptr msg) {
#if DEBUG
	printf("[on_message_bitmex]\n");
#endif
	static char netmsg[BUFFERSIZE];
	static char timemsg[9];

        string str = msg->get_payload();
        string price_str, size_str, side_str, time_str, symbol_str;
        json jtable = json::parse(str.c_str());

	static CSKClients* pClients = CSKClients::GetInstance();
	if(pClients == NULL)
        	throw "GET_CLIENTS_ERROR";
        for(int i=0 ; i<jtable["data"].size() ; i++)
        {
                memset(netmsg, 0, BUFFERSIZE);
                memset(timemsg, 0, 8);
                static int tick_count=0;
                time_str   = jtable["data"][i]["timestamp"];
		symbol_str = jtable["data"][i]["symbol"];
                price_str  = to_string(jtable["data"][i]["price"]);
                size_str   = to_string(jtable["data"][i]["size"]);
                sprintf(timemsg, "%.2s%.2s%.2s%.2s", time_str.c_str()+11, time_str.c_str()+14, time_str.c_str()+17, time_str.c_str()+20);
                sprintf(netmsg, "01_ID=%s.BMEX,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,",
			symbol_str.c_str(), timemsg, price_str.c_str(), size_str.c_str(), tick_count++, pClients->m_strEPIDNum.c_str());
                int msglen = strlen(netmsg);
#ifdef DEBUG
//		cout << setw(4) << jtable << endl;
		cout << netmsg << endl;
#endif
                netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_1;
                netmsg[strlen(netmsg)] = GTA_TAIL_BYTE_2;

		vector<shared_ptr<CSKClient> >::iterator iter = pClients->m_vClient.begin();

		while(iter != pClients->m_vClient.end())
		{
			CSKClient* pClient = (*iter).get();
			if(pClient->GetStatus() == csOffline && (*iter).unique()) {
				iter++;
				continue;
			}
			pClient->SendAll(NULL, netmsg, strlen(netmsg));
			iter++;
		}

        }
}
context_ptr CB_TLS_Init(const char * hostname, websocketpp::connection_hdl) {
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
    return ctx;
}

