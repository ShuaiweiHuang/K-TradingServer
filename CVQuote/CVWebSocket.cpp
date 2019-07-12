#include "CVWebSocket.h"
#include "CVServerManager.h"
#include "CVServer.h"
#include "CVGlobal.h"
using namespace std;

void on_message_binance(websocketpp::connection_hdl, client::message_ptr msg) {
#if DEBUG
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
	cout << jtable["T"]<<" " ;
#if DEBUG
	//std::cout << std::setw(4) << jtable << " ";
	std::cout << std::setw(4) << symbol_str << " ";
	std::cout << std::setw(4) << price_str << " ";
	std::cout << std::setw(4) << size_str << endl;
#endif
	int size_int = stof(size_str) * 1000000;
	size_str = to_string(size_int);

	sprintf(netmsg, "01_ID=%s.BINANCE,Time=%s,C=%s,V=%s,TC=%d,EPID=%s,",
		symbol_str.c_str(), time_str.c_str(), price_str.c_str(), size_str.c_str(), tick_count_binance++, pClients->m_strEPIDNum.c_str());

	int msglen = strlen(netmsg);
	netmsg[strlen(netmsg)] = 0x0D;
	netmsg[strlen(netmsg)] = 0x0A;

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


void on_message_bitmex(websocketpp::connection_hdl, client::message_ptr msg) {
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
//		std::cout << std::setw(4) << jtable << endl;
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
                netmsg[strlen(netmsg)] = 0x0D;
                netmsg[strlen(netmsg)] = 0x0A;

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

bool verify_subject_alternative_name(const char * hostname, X509 * cert) {

	STACK_OF(GENERAL_NAME) * san_names = NULL;
	san_names = (STACK_OF(GENERAL_NAME) *) X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
	if (san_names == NULL) {
		return false;
	}
   
	int san_names_count = sk_GENERAL_NAME_num(san_names);
    
	bool result = false;
    
	for (int i = 0; i < san_names_count; i++) {
		const GENERAL_NAME * current_name = sk_GENERAL_NAME_value(san_names, i);

		if (current_name->type != GEN_DNS) {
			continue;
		}
		char * dns_name = (char *) ASN1_STRING_data(current_name->d.dNSName);
		// Make sure there isn't an embedded NUL character in the DNS name
		if (ASN1_STRING_length(current_name->d.dNSName) != strlen(dns_name)) {
			break;
	
		// Compare expected hostname with the CN
		result = (strcasecmp(hostname, dns_name) == 0);
	}
	sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);
    
	return result;
}

/// Verify that the certificate common name matches the given hostname
bool verify_common_name(const char * hostname, X509 * cert) {
	// Find the position of the CN field in the Subject field of the certificate
	int common_name_loc = X509_NAME_get_index_by_NID(X509_get_subject_name(cert), NID_commonName, -1);
	if (common_name_loc < 0) {
		return false;
	}
    
	// Extract the CN field
	X509_NAME_ENTRY * common_name_entry = X509_NAME_get_entry(X509_get_subject_name(cert), common_name_loc);
	if (common_name_entry == NULL) {
		return false;
	}
    
	// Convert the CN field to a C string
	ASN1_STRING * common_name_asn1 = X509_NAME_ENTRY_get_data(common_name_entry);
	if (common_name_asn1 == NULL) {
		return false;
	}

	char * common_name_str = (char *) ASN1_STRING_data(common_name_asn1);
    
	// Make sure there isn't an embedded NUL character in the CN
	if (ASN1_STRING_length(common_name_asn1) != strlen(common_name_str)) {
		return false;
	}
    
	// Compare expected hostname with the CN
	return (strcasecmp(hostname, common_name_str) == 0);
}

bool verify_certificate(const char * hostname, bool preverified, boost::asio::ssl::verify_context& ctx) {
	int depth = X509_STORE_CTX_get_error_depth(ctx.native_handle());

	if (depth == 0 && preverified) {
		X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        
		if (verify_subject_alternative_name(hostname, cert)) {
			return true;
		} else if (verify_common_name(hostname, cert)) {
			return true;
		} else {
			return false;
		}
	}

	return preverified;
}

context_ptr on_tls_init(const char * hostname, websocketpp::connection_hdl) {
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
    return ctx;
}
