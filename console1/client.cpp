// console1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include "../json.hpp"

#pragma comment(lib,"ws2_32.lib")

using namespace std;

struct sockaddr_in server;
WSADATA wsa;
SOCKET sckt;

int main()
{
	cout << "Initializing WinSock..." << endl;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		cout << "Failed. Error Code: " << WSAGetLastError() << endl;
		return 1;
	}
	cout << "Initialized." << endl;

	if ((sckt = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		cout << "Could not create socket: " << WSAGetLastError() << endl;
	}
	cout << "Socket created." << endl;

	InetPton(AF_INET, _T("127.0.0.1"), &server.sin_addr);
	//server.sin_addr.S_un.S_addr = inet_addr("74.125.235.20");
	server.sin_family = AF_INET;
	server.sin_port = htons(8888);

	if (connect(sckt, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		cout << "Connection error" << endl;
		return 1;
	}
	cout << "Connected." << endl;


	
    return 0;
}

