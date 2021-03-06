// server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include "../json.hpp"
#include "Message.h"

#pragma comment(lib,"ws2_32.lib")

using namespace std;
using namespace nlohmann;

#define MAXRECV 1200
#define MAXRESP 1000000
#define MAXCLIENTS 30
#define ENDCHAR '\n'

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

string client_names[MAXCLIENTS];
string history[MAXCLIENTS];

map<string, vector<Message>> messages;

bool WaitForConnection();
string GetResponse(string request, int i);
bool HandleRequest(int recvlen, int i);
string GetBadResponse(string err);
string GetGoodResponse(json content);
string TryLogin(json request, int i);

int main()
{
	FD_ZERO(&readfds);

	for (int i = 0; i < MAXCLIENTS; i++)
	{
		client_socket[i] = 0;
		history[i] = "";
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
						continue;
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
					HandleRequest(recvlen, i);
					/// RESPONSE ENDS HERE   ===============================================================
				}
			}
		}
		/*
		// BULLSHIT BELOW
		// =================================================================================================

		//len = recv(resp_sckt, recvbuf, recvbuflen, 0);
		
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

bool HandleRequest(int recvlen, int i) {
	cout << "Bytes received: " << recvlen << endl;

	string request = "";

	if (recvlen > 0)
		request = string(recvbuf, recvlen);

	history[i].append(request);
	if (request.find(ENDCHAR) == string::npos)
	{
		return true;
	}

	string response = GetResponse(history[i], i);
	history[i].clear();

	//strncpy_s(respbuf, response.c_str(), response.length());

	int resplen = send(s, response.c_str(), response.length(), 0);

	if (resplen == SOCKET_ERROR)
	{
		cout << "Response failed with error code: " << WSAGetLastError() << endl;
		closesocket(s);
		return false;
	}
	cout << "Bytes sent: " << resplen << endl;
	return true;
}

string GetResponse(string request, int i) {
	//actions: login, sendMsg, listMsg
	if (request == "")
		return GetBadResponse("Empty request.");
	if (!json::accept(request))
		return GetBadResponse("Invalid json.");
	json requestjson = json::parse(request);
	cout << "Request: " << requestjson << endl;

	if (requestjson.count("request") == 0)
	{
		return GetBadResponse("Invalid json format: action not specified.");
	}

	string req = requestjson["request"];

	if (req == "login")
		return TryLogin(requestjson, i);
	if (req == "sendMsg")
		return "s";
	if (req == "listMsg")
		return "m";
	return GetBadResponse("Invalid action specified.");
}

string GetBadResponse(string err)
{
	json resp;
	resp["response"] = "FAIL";
	resp["error"] = err;

	return resp.dump();
}

string GetGoodResponse(json content)
{
	json res;
	res["response"] = "OK";
	if (!content.empty())
		res["data"] = content;
	return res.dump();
}

string TryLogin(json request, int i)
{
	if (request.count("username") == 0)
		return GetBadResponse("No username specified.");
	string name = request["username"];
	for (int j = 0; j < MAXCLIENTS; j++)
	{
		if (j != i && client_names[j] == name)
			return GetBadResponse("Username already taken.");
	}
	client_names[i] = name;
	return GetGoodResponse(json());
}