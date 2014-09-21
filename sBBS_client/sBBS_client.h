//sBBS_client.h
#include <string>
using namespace std;

class sBBS_client
{
public:
	SOCKET sock;

	sBBS_client(string remote_ip, UINT remote_ptn);
	~sBBS_client();
	int startConnection(void);
	int stopConnection(void);
	int sendMsg(string msg_send);
	int client_proc(void);
private:
	unsigned long remote_ipaddr;
	UINT remote_portNum;

};
