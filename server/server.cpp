// server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include "../json.hpp"

#pragma comment(lib,"ws2_32.lib")

using namespace std;

#define MAXRECV 1200
#define MAXRESP 1200
#define MAXCLIENTS 30

int recvbuflen = MAXRECV;
char recvbuf[MAXRECV + 1];
int respbuflen = MAXRESP;
char respbuf[MAXRESP + 1];
int activity;

struct sockaddr_in server;
struct sockaddr_in client;
WSADATA wsa;
SOCKET sckt;
SOCKET resp_sckt;
SOCKET client_socket[MAXCLIENTS], s;
fd_set readfds;

bool WaitForConnection();

int main()
{
	FD_ZERO(&readfds);

	for (int i = 0; i < 30; i++)
	{
		client_socket[i] = 0;
	}

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

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);
	//bind(sckt, (struct sockaddr *)&server, (int)1);
	//int res = 1;
	if (bind(sckt, (const struct sockaddr *) &server, (int)sizeof(server)) == SOCKET_ERROR)
	{
		cout << "Bind failed with error code: " << WSAGetLastError() << endl;
	}

	cout << "Bind done" << endl;

	listen(sckt, SOMAXCONN);

	while (WaitForConnection());
	closesocket(s);
	WSACleanup();
    return 0;
}


bool WaitForConnection() {
	cout << "Waiting for incoming connections..." << endl;
	int len = sizeof(struct sockaddr_in);
	
	FD_ZERO(&readfds);
	FD_SET(sckt, &readfds);

	//add child sockets to fd set
	for (int i = 0; i < MAXCLIENTS; i++)
	{
		s = client_socket[i];
		if (s > 0)
		{
			FD_SET(s, &readfds);
		}
	}

	activity = select(0, &readfds, NULL, NULL, NULL);

	if (activity == SOCKET_ERROR)
	{
		cout << "Select failed with error code: " << WSAGetLastError() << endl;
	}
	else
	{
		if (FD_ISSET(sckt, &readfds))
		{
			resp_sckt = accept(sckt, (struct sockaddr *)&client, &len);
			cout << "Connection found." << endl;

			if (resp_sckt == INVALID_SOCKET)
			{
				cout << "Failed to accept connection with error code: " << WSAGetLastError() << endl;
			}

			char ip[INET_ADDRSTRLEN];
			const char* result = inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip));
			if (result == 0) {
				cout << "Failed to convert address to string. Error code:" << errno << endl;
			}

			cout << "Connection accepted. Socket fd is " << resp_sckt << ", ip: " << ip << ", port: " << ntohs(client.sin_port) << endl;
			if (send(resp_sckt, "Welcome", strlen("Welcome"), 0) != strlen("Welcome"))
			{
				cout << "Welcome message failed." << endl;
			}
			else
			{
				cout << "Welcome message sent successfully." << endl;
				for (int i = 0; i < MAXCLIENTS; i++)
				{
					if (client_socket[i] == 0)
					{
						client_socket[i] = resp_sckt;
						cout << "Adding to list of sockets: " << i << endl;
						break;
					}
				}
			}
		}

		//already existing connection
		for (int i = 0; i < MAXCLIENTS; i++)
		{
			s = client_socket[i];
			if (FD_ISSET(s, &readfds))
			{
				getpeername(s, (struct sockaddr*) &client, (int*)&len);
				int recvlen = recv(s, recvbuf, recvbuflen, 0);
				if (recvlen == SOCKET_ERROR)
				{
					int err = WSAGetLastError();
					if (err == WSAECONNRESET)
					{
						char ip[INET_ADDRSTRLEN];
						const char* result = inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip));
						if (result == 0) {
							cout << "Failed to convert address to string. Error code:" << errno << endl;
						}
						cout << "Unexpected disconnect; ip: " << ip << ", port: " << ntohs(client.sin_port) << endl;
						closesocket(s);
						client_socket[i] = 0;
					}
					else
					{
						cout << "Recv failed with error code: " << err << endl;
					}
				}
				if (recvlen == 0)
				{
					char ip[INET_ADDRSTRLEN];
					const char* result = inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip));
					if (result == 0) {
						cout << "Failed to convert address to string. Error code:" << errno << endl;
					}
					cout << "Host disconnected , ip: " << ip << ", port: " << ntohs(client.sin_port) << endl;
					closesocket(s);
					client_socket[i] = 0;
				}
				else
				{
					/// RESPONSE BEGINS HERE ===============================================================
					cout << "Bytes received: " << recvlen << endl;

					string response;
					if (recvbuf == "request")
						response = "response";
					else
						response = "wrong request";
					strncpy_s(respbuf, response.c_str(), response.length());

					int resplen = send(s, respbuf, response.length(), 0);

					if (resplen == SOCKET_ERROR)
					{
						cout << "Response failed with error code: " << WSAGetLastError() << endl;
						closesocket(s);
					}
					else
					{
						cout << "Bytes sent: " << resplen << endl;
					}
					/// RESPONSE ENDS HERE   ===============================================================
				}
			}
		}

		// BULLSHIT BELOW
		// =================================================================================================

		//len = recv(resp_sckt, recvbuf, recvbuflen, 0);
		/*
		if (len < 0)
		{
			cout << "recv failed with error code: " << WSAGetLastError() << endl;
			closesocket(resp_sckt);
		}
		else
		{
			cout << "Bytes received: " << len << endl;

			string response;
			if (recvbuf == "request")
				response = "response";
			//strncpy(respbuf, "response", sizeof(respbuf) - 1);
			else
				response = "wrong request";
			//strncpy(respbuf, "wrong request", sizeof(respbuf) - 1);

			strncpy_s(respbuf, response.c_str(), response.length());

			len = send(resp_sckt, respbuf, response.length(), 0);

			if (len == SOCKET_ERROR)
			{
				cout << "Response failed with error code: " << WSAGetLastError() << endl;
				closesocket(resp_sckt);
			}
			else
			{
				cout << "Bytes sent: " << len << endl;
			}
		}
		*/
	}

	return true;
}