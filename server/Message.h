#pragma once
#include <string>
#include <ctime>
using namespace std;
struct Message
{
public:
	Message(time_t time, string sender, string subject, string content, string receiver);
	Message(string sender, string subject, string content, string receiver) : Message(time(0), sender, subject, content, receiver) {}
	~Message();

	time_t Time;
	string Sender;
	string Subject;
	string Content;
	string Receiver;
};

