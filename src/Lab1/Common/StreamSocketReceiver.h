#pragma once

#include <iostream>
#include <WinSock2.h>

using namespace std;

class StreamSocketReceiver
{
private:
	size_t packet_size;
public:
	StreamSocketReceiver(size_t packet_size);
	~StreamSocketReceiver();

	void receive(SOCKET s, ostream &stream);
	size_t receiveHeader(SOCKET s);
	void receivePayload(SOCKET s, ostream &stream, size_t len);
};

