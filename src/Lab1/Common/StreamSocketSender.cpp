#include "StreamSocketSender.h"
#include "IncompleteSendException.h"
#include <WinSock2.h>
#include <iostream>
#include <malloc.h>

StreamSocketSender::StreamSocketSender(size_t packet_size)
{
	this->packet_size = packet_size;
}


StreamSocketSender::~StreamSocketSender()
{
}

void StreamSocketSender::send(SOCKET s, istream &stream, size_t len)
{
	this->sendHeader(s, len);
	this->sendPayload(s, stream, len);
}

void StreamSocketSender::sendHeader(SOCKET s, size_t len)
{
	char header[21];
	sprintf_s(header, "%020lu", len);
	size_t bytes_sent = ::send(s, header, 20, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != 20) {
		throw IncompleteSendException(bytes_sent, 20);
	}
}

void StreamSocketSender::sendPayload(SOCKET s, istream &strm, size_t len)
{
	size_t bytes_sent = 0;
	void *buf = malloc(this->packet_size);
	while (bytes_sent < len) {
		size_t bytes_to_read = min(this->packet_size, len - bytes_sent);
		strm.read((char*)buf, bytes_to_read);
		streamsize bytes_read = strm.gcount();
		if (bytes_read != bytes_to_read) {
			// Abort; could not read enough bytes from input stream
			free(buf);
			throw IncompleteSendException(bytes_sent, len);
		}
		size_t chunk_sent = ::send(s, (char*)buf, bytes_read, 0);
		if (chunk_sent == SOCKET_ERROR || chunk_sent != bytes_read) {
			free(buf);
			throw IncompleteSendException(bytes_sent, len);
		}
		bytes_sent += bytes_read;
	}
	free(buf);
}