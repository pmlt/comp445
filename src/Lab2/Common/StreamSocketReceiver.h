#pragma once

#include <iostream>
#include "PhilSock.h"

using namespace std;

class StreamSocketReceiver
{
private:
	size_t packet_size;
public:
	StreamSocketReceiver(size_t packet_size);
	~StreamSocketReceiver();

	void receive(net::Socket &s, ostream &stream);
	size_t receiveHeader(net::Socket &s);
	void receivePayload(net::Socket &s, ostream &stream, size_t len);
};

