//sBBS_server.h
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

class sBBS_server
{
public:
	SOCKET main_sock;

	sBBS_server(UINT ptn);
	~sBBS_server();
	int startService(void);
	int stopService(void);
	int sendMsg(SOCKET s, string msg_send);
	int initLogFile(void);
	int closeLogFile(void);
	int server_multiThread(SOCKET s);
	static DWORD WINAPI server_proc(sBBS_server* class_ptr, SOCKET s);

private:
	UINT portNum;
	struct User
	{
		char userID[32];
		char password[32];
		char* ipAddress;
		bool root;
		bool log_state;
	};
	ifstream ifd;// log file handler
	ofstream ofd;
	//filebuf buffer;
	//ostream output;
	//istream input;
	int userCount = 4;// user count
	User userList[4];// user list
	string messages;// log messages

};

