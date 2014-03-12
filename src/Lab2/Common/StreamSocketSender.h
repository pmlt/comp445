#pragma once

#include <iostream>
#include <WinSock2.h>

using namespace std;

class StreamSocketSender
{
private:
	size_t packet_size;

public:
	StreamSocketSender(size_t packet_size);
	~StreamSocketSender();

	void send(SOCKET s, istream &stream, size_t len);
	void sendHeader(SOCKET s, size_t len);
	void sendPayload(SOCKET s, istream &stream, size_t len);
};

