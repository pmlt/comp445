#pragma once

#include <iostream>
#include "PhilSock.h"

using namespace std;

class StreamSocketSender
{
private:
	size_t packet_size;

public:
	StreamSocketSender(size_t packet_size);
	~StreamSocketSender();

	void send(net::Socket &s, istream &stream, size_t len);
	void sendHeader(net::Socket &s, size_t len);
	void sendPayload(net::Socket &s, istream &stream, size_t len);
};

