#include "StreamSocketReceiver.h"
#include "IncompleteReceiveException.h"
#include "UnparseableHeaderException.h"
#include "PhilSock.h"
#include <malloc.h>

StreamSocketReceiver::StreamSocketReceiver(size_t packet_size)
{
	this->packet_size = packet_size;
}

StreamSocketReceiver::~StreamSocketReceiver()
{
}

void StreamSocketReceiver::receive(net::Socket &s, ostream &stream)
{
	size_t len = this->receiveHeader(s);
	this->receivePayload(s, stream, len);
}

size_t StreamSocketReceiver::receiveHeader(net::Socket &s)
{
	char buf[20];
	size_t bytes_received;
	if (bytes_received = s.recv(buf, 20, 0) == SOCKET_ERROR) {
		throw IncompleteReceiveException(bytes_received, 20);
	}

	// Try to parse length of payload in header
	size_t len;
	if (EOF == sscanf_s(buf, "%20lu", &len)) {
		throw UnparseableHeaderException(buf);
	}
	return len;
}

void StreamSocketReceiver::receivePayload(net::Socket &s, ostream &stream, size_t len)
{
	void *buf = malloc(this->packet_size);
	size_t bytes_read = 0;
	while (bytes_read < len) {
		size_t exp_bytes = min(this->packet_size, len - bytes_read);
		size_t bytes_recv = s.recv((char*)buf, exp_bytes, 0);
		if (bytes_recv == SOCKET_ERROR || bytes_recv != exp_bytes) {
			free(buf);
			throw IncompleteReceiveException(bytes_read + bytes_recv, len);
		}
		bytes_read += bytes_recv;
		stream.write((char*)buf, bytes_recv);
	}
	free(buf);
}
