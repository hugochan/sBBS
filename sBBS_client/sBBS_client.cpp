#include <winsock.h>
#include <iostream>
#include <fstream>
#include <string>
#include <conio.h>
#include "sBBS_client.h"

#pragma comment(lib,"wsock32.lib")

sBBS_client::sBBS_client(string remote_ip, UINT remote_ptn)
{
	//reset sock number
	sock = 0;

	// tackle ip address
	// exception handling to be added...
	remote_ipaddr = inet_addr(remote_ip.c_str());

	//check port num
	if (remote_ptn < 0 || remote_ptn > 65535)
	{
		cerr << "port num error !" << endl;
		system("pause");
		exit(1);
	}
	remote_portNum = remote_ptn;
}

sBBS_client::~sBBS_client()
{
}

int sBBS_client::startConnection(void)
{
	int retval;
	sockaddr_in client;
	WSAData wsa;
	WSAStartup(0x101, &wsa);

	// check sock state
	if (sock != 0)
	{
		cout << "connection has been established !" << endl;
		return 0;
	}
	sock = socket(AF_INET, SOCK_STREAM, 0);

	client.sin_family = AF_INET;
	client.sin_addr.S_un.S_addr = remote_ipaddr;
	client.sin_port = htons(remote_portNum);
	retval = connect(sock, (sockaddr*)&client, sizeof(client));
	
	// check connection state
	if (retval == -1)
	{
		retval = WSAGetLastError();
		if (retval != WSAEWOULDBLOCK)
		{
			closesocket(sock);
			sock = 0;
			WSACleanup();
			return -1;
		}
		else
		{
		}
	}
	else{
	}
	return 0;
}

int sBBS_client::stopConnection(void)
{
	// check sock state
	if (sock == 0)
	{
		cout << "connection hasn`t been started !" << endl;
		return 0;
	}
	closesocket(sock);
	sock = 0;
	WSACleanup();
	cout << "stop the connection !" << endl;
	return 0;
}

int sBBS_client::sendMsg(string msg_send)
{
	int len, retval;
	if (sock == 0)
	{
		cerr << "connection hasn`t been started !" << endl;
		return -1;
	}
	len = msg_send.length();
	retval = send(sock, const_cast<char*>(msg_send.c_str()), len, 0);
	if (retval == -1)
	{
		retval = WSAGetLastError();
		if (retval != WSAEWOULDBLOCK && retval != WSAECONNRESET)
		{
			closesocket(sock);
			sock = 0;
			WSACleanup();
			cerr << "send() failed !" << endl;
		}
	}
	return 0;
}

int sBBS_client::client_proc(void)
{
	fd_set readfds;
	bool keyboard_state = true;// keyboard event always comes first for a client
	int retval, len;
	char recv_buf[1024];
	char send_buf[1024];
	unsigned long mode = 1;// set nonblocking mode
	const timeval timeout{0, 100000};
	FD_ZERO(&readfds);
	retval = ioctlsocket(sock, FIONBIO, &mode);
	if (retval == SOCKET_ERROR)
	{
		retval = WSAGetLastError();
		return -1;
	}

	cout << "c:";
	while (1)
	{
		if (keyboard_state == true)
		{
			if (kbhit())// querying keyboard event
			{
				if (cin.getline(send_buf, sizeof(send_buf)))
				{
					sendMsg(send_buf + string("\n"));
					keyboard_state = false;
				}
			}
		}
		else
		{
			FD_SET(sock, &readfds);
			retval = select(0, &readfds, NULL, NULL, 0);// blocking
			if (retval == SOCKET_ERROR)
			{
				retval = WSAGetLastError();
				break;
			}

			if (retval = FD_ISSET(sock, &readfds))
			{
				len = recv(sock, recv_buf, sizeof(recv_buf), 0);

				if (len <= 0)
				{
					retval = WSAGetLastError();
					if (retval != WSAEWOULDBLOCK && retval != WSAECONNRESET)
					{
						cerr << "recv() failed !" << endl;
						break;
					}
					break;
				}
				recv_buf[len] = 0;
				cout << string("s:") + recv_buf << endl;
				keyboard_state = true;
				if (string(send_buf) == "quit" && string(recv_buf) == "200 OK")
				{
					cout << "c:quit..." << endl;
					break;
				}
				if (string(send_buf) == "shutdown" && string(recv_buf) == "200 OK")
				{
					cout << "c:server shutdown..." << endl;
					break;
				}
				cout << "c:";
			}
		}
	}

	closesocket(sock);
	sock = 0;
	WSACleanup();
	cout <<"close sockets" << endl;
	return 0;
}

int main(int argc, char** argv)
{
	int retval;
	sBBS_client my_client("127.0.0.1", 8000);
	if (my_client.startConnection() == 0)// connect sucess
	{
		cout << "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+"<< endl;
		cout << "+-+-+-+-+-+    Simple Bulletin Board System   +-+-+-+-+-+" << endl;
		cout << "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+" << endl;
		cout << "Commands allowed by the server (s) for this client (c):" << endl;
		cout << "display" << endl;
		cout << "post" << endl;
		cout << "shutdown" << endl;
		cout << "login" << endl;
		cout << "logout" << endl;
		cout << "quit" << endl;
		cout << "who" << endl;

		my_client.client_proc();
	}
	else
	{
		cout << "connect error !" << endl;
	}
	system("pause");
}