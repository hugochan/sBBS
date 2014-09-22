#include <winsock.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include "sBBS_server.h"
#include <stdlib.h>

#pragma comment(lib,"wsock32.lib")

DWORD __stdcall startMethodInThread(LPVOID arg);

enum LogState { login, logout };
enum RootState { yes, no };
enum commandName { Display, Post, ShutDown, Login, Logout, Quit, Who };
const char* command_array[7] = { "display", "post", "shutdown", "login", "logout", "quit", "who" };
struct myParam
{
	SOCKET sock;
	sBBS_server* class_ptr;
};

using namespace std;

sBBS_server::sBBS_server(UINT ptn)
{
	//initialize user list
	userList[0] = { "root", "root01", true, false };
	userList[1] = { "john", "john01", false, false };
	userList[2] = { "david", "david01", false, false };
	userList[3] = { "mary", "mary01", false, false };

	//reset sock number
	main_sock = 0;

	//check port num
	if (ptn < 0 || ptn > 65535)
	{
		cerr << "port num error !" << endl;
		system("pause");
		exit(1);
	}
	portNum = ptn;
	// initialize log file
	if (initLogFile() == -1)
	{
		cerr << "log file initialization error !" << endl;
		system("pause");
		exit(1);
	}
}

sBBS_server::~sBBS_server()
{
}

int sBBS_server::startService(void)
{
	int retval;
	sockaddr_in server;
	WSAData wsa;
	WSAStartup(0x101, &wsa);

	if (main_sock != 0){
		cout << "server has been started !" << endl;
		return 0;
	}

	main_sock = socket(AF_INET, SOCK_STREAM, 0);

	server.sin_family = AF_INET;
	server.sin_addr.S_un.S_addr = htonl(INADDR_ANY);// bind all the ip addresses
	server.sin_port = htons(portNum);

	retval = bind(main_sock, (sockaddr*)&server, sizeof(server));
	retval = listen(main_sock, 1024);
	if (retval == -1)
	{
		cerr << "server listen error !" << endl;
		system("pause");
		exit(1);
	}
	return 0;
}

int sBBS_server::stopService(void)
{
	closesocket(main_sock);
	main_sock = 0;
	//closeLogFile();
	WSACleanup();
	return 0;
}

int sBBS_server::sendMsg(SOCKET s, string msg_send)
{
	int retval, len;
	if (main_sock == 0)
	{
		cerr << "server hasn`t been started !" << endl;
		return -1;
	}
	else if (s == 0)
	{
		cerr << "no connection is established !" << endl;
		return -1;
	}
	else
	{
		len = msg_send.length();
		retval = send(s, const_cast<char*>(msg_send.c_str()), len, 0);
		if (retval == -1)
		{
			retval = WSAGetLastError();
			if (retval != WSAEWOULDBLOCK)
			{
				closesocket(s);// close sub-socket
				cerr << "send() failed !" << endl;
			}
		}
	}
	return 0;

}

int sBBS_server::initLogFile(void)
{
	int begin, end, size;
	char* memblock;
	ifd.open("messages.log", ios::in);
	if (!ifd.good())
	{
		return -1;
	}
	else
	{
		begin = ifd.tellg();
		ifd.seekg(0, ios::end);
		end = ifd.tellg();
		size = end - begin;

		if (size < 0)
		{
			ifd.close();
			ifd.clear();
			return -1;
		}
		else if (size == 0)
		{
			return 0;
		}
		else
		{
			memblock = new char[size];
			ifd.seekg(0, ios::beg);
			ifd.read(memblock, size);
			messages = string(memblock);
			ifd.close();
			return 0;
		}

	}
}

int sBBS_server::closeLogFile(void)
{
	//fd.close();
	return 0;
}

int sBBS_server::server_multiThread(SOCKET s)
{
	DWORD dwThreadId;
	HANDLE hThread;

	myParam* my_param = new myParam;
	my_param->sock = s;
	my_param->class_ptr = this;

	hThread = CreateThread(
		NULL,                        // no security attributes 
		0,                           // use default stack size  
		startMethodInThread,                  // thread function 
		my_param,                // argument to thread function 
		0,                           // use default creation flags 
		&dwThreadId);                // returns the thread identifier
	return 0;
}

DWORD WINAPI sBBS_server::server_proc(sBBS_server* class_ptr, SOCKET s)
{
	LogState log_state = logout;
	RootState root_state = no;
	char recv_buf[1024];
	int retval, len, userIndex, i;
	bool post_sub_state = false;
	string errMsg = "300 message format error.";

	unsigned long mode = 0;
	int iResult = ioctlsocket(s, FIONBIO, &mode); // block the socket ???
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);


	while (1)
	{
		len = recv(s, recv_buf, sizeof(recv_buf), 0);

		if (len <= 0)
		{
			retval = WSAGetLastError();
			if (retval != WSAEWOULDBLOCK)
			{
//				closesocket(s);
				//cerr << "recv() failed !" << endl;
	//			return -1;
			}
			//return -1;
		}
		else
		{
			recv_buf[len-1] = 0;// the last character is '\n'
			string msg(recv_buf);
			cout << string("c:") + recv_buf << endl;
			memset(recv_buf, 0, sizeof(recv_buf));

			if (msg == command_array[Display])
			{

			}
			else if (msg == command_array[Post] || post_sub_state == true)//{ Display, Post, ShutDown, Login, Logout, Quit, Who };
			{
				if (log_state == login && post_sub_state == false)
				{
					class_ptr->sendMsg(s, string("200 OK"));
					cout << "s:200 OK" << endl;
					post_sub_state = true;
				}
				else if (log_state == logout && post_sub_state == false)
				{
					class_ptr->sendMsg(s, string("401 You are not currently logged in, login first"));
					cout << "s:401 You are not currently logged in, login first" << endl; 
				}
				else// post_sub_state == true
				{
					class_ptr->messages += (msg + "\n");
					// dealing with log file
					class_ptr->ofd.open("messages.log", ios::out);
					if (class_ptr->ofd.good())
					{
						class_ptr->ofd.write(const_cast<char*>(class_ptr->messages.c_str()), class_ptr->messages.length());
					}
					class_ptr->ofd.close();

					class_ptr->sendMsg(s, string("200 OK"));
					cout << "s:200 OK" << endl;
					post_sub_state = false;
				}
			}
			else if (msg == command_array[ShutDown])
			{
				if (root_state == yes)
				{
					cout << "s:200 OK" << endl;
					class_ptr->sendMsg(s, string("200 OK"));
					Sleep(10);// miliseconds
					class_ptr->stopService();// close sockets
					//class_ptr->closeLogFile();// close log file
					return 0;
				}
				else if (root_state == no)
				{
					class_ptr->sendMsg(s, string("402 User not allowed to execute this command."));
					cout << "s:402 User not allowed to execute this command." << endl;
				}
				else
				{

				}
			}
			else if ((msg.substr(0, msg.find(" ")) == command_array[Login])&&msg.length()>6)// rule out the case "login"
			{
				if (log_state == logout)
				{
					const char* delim = " ";
					char * tmp = new char;
					char* p = new char;
					p = strtok(const_cast<char*>(msg.c_str()), delim);
					tmp = "login";
					if (strcmp(p, tmp) == 0)
					{
						p = strtok(NULL, delim);
						for (i = 0; i < class_ptr->userCount; i++)
						{
							if (strcmp(p, class_ptr->userList[i].userID) == 0)
							{
								p = strtok(NULL, delim);
								if (strcmp(p, class_ptr->userList[i].password) == 0)
								{
									userIndex = i;
									log_state = login;
									class_ptr->userList[userIndex].log_state = true;
									if (class_ptr->userList[userIndex].root == true)
									{
										root_state = yes;
									}
									class_ptr->sendMsg(s, string("200 OK"));
									cout << "s:200 OK" << endl;
									break;
								}
							}
						}
						if (log_state == logout)
						{
							class_ptr->sendMsg(s, string("410 Wrong UserID or Password"));
							cout << "s:410 Wrong UserID or Password" << endl;
						}
					}
				}
				else
				{
					class_ptr->sendMsg(s, string("420 You are currently logged in, don`t login again"));
					cout << "s:420 You are currently logged in, don`t login again" << endl;
				}
			}
			else if (msg == command_array[Logout])
			{
				if (log_state == login)
				{
					log_state = logout;
					class_ptr->userList[userIndex].log_state = false;
					class_ptr->sendMsg(s, string("200 OK"));
					cout << "s:200 OK" << endl;
				}
				else
				{
					class_ptr->sendMsg(s, string("401 You are not currently logged in, login first"));
					cout << "s:401 You are not currently logged in, login first" << endl;
				}
			}
			else if (msg == command_array[Quit])
			{

			}
			else if (msg == command_array[Who])
			{
				string active_users("");
				for (i = 0; i < class_ptr->userCount; i++)
				{
					if (class_ptr->userList[i].log_state == true)
					{
						// ip address?
						active_users += (class_ptr->userList[i].userID + string("\n"));

					}
				}
				class_ptr->sendMsg(s, "200 OK\nThe list of the active users:\n" + active_users);
				cout << "s:200 OK\nThe list of the active users:\n" + active_users << endl;
			}
			else
			{
				class_ptr->sendMsg(s, errMsg);
				cout << "s:" + errMsg << endl;
			}
		}
	}
	return 0;
}

DWORD __stdcall startMethodInThread(LPVOID arg)
{
	if (!arg)
	{
		return -1;
	}
	myParam* arg_ptr = (myParam*)arg;
	sBBS_server *class_ptr = arg_ptr->class_ptr;
	class_ptr->server_proc(class_ptr, arg_ptr->sock);
	return 0;
}

int main(int argc, char** argv)
{
	sockaddr_in client_addr;
	int len = sizeof(client_addr);

	sBBS_server my_server(8000);
	my_server.startService();
/*	SOCKET sub_sock = accept(my_server.main_sock, (struct sockaddr *)&client_addr, &len);
	while (1){
		int retval = my_server.sendMsg(sub_sock, string("200 OK"));
	}
*/
	while (1)
	{
		SOCKET sub_sock = accept(my_server.main_sock, (struct sockaddr *)&client_addr, &len);
		if (sub_sock != -1)
		{
			my_server.server_multiThread(sub_sock);
		}
	}

	/*
	sockaddr_in caddr;
	int len = sizeof(caddr);
	my_server.sub_sock = accept(my_server.main_sock, (struct sockaddr *)&caddr, &len);
	char buf[20];
	len = recv(my_server.sub_sock, buf, strlen(buf), 0);
	cout << buf << endl;
	*/
	
	system("pause");
}