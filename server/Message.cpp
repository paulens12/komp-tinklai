#include "stdafx.h"
#include "Message.h"
#include <ctime>

Message::Message(time_t time, string sender, string subject, string content, string receiver)
{
	Sender = sender;
	Subject = subject;
	Content = content;
	Receiver = receiver;
	Time = time;
}


Message::~Message()
{
	free(&Sender);
	free(&Subject);
	free(&Content);
	free(&Receiver);
	free(&Time);
}
