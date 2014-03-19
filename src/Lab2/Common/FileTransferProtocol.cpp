#include "FileTransferProtocol.h"
#include "PhilSock.h"

FileTransferProtocol::FileTransferProtocol()
{
}


FileTransferProtocol::~FileTransferProtocol()
{
}

int FileTransferProtocol::sendFilename(net::Socket &s, string filename) {
	int bytes_sent;
	char fNameHeader[4];
	sprintf_s(fNameHeader, "%03lu", (unsigned long)filename.size());
	bytes_sent = s.send(fNameHeader, 3, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != 3) {
		onTransferError("Could not send filename size");
		return -1;
	}
	bytes_sent = s.send(filename.c_str(), filename.size(), 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != filename.size()) {
		onTransferError("Could not send filename");
		return -1;
	}
	return 0;
}

int FileTransferProtocol::recvFilename(net::Socket &s, string &filename) {
	int bytes_received;

	// First, receive the filename size (3 bytes ascii-encoded unsigned integer)
	char buf[3];
	bytes_received = s.recv(buf, 3, 0);
	if (bytes_received == SOCKET_ERROR || bytes_received != 3) {
		onTransferError("Could not receive filename header size!");
		return -1;
	}

	// Try to parse what we have received
	size_t len;
	if (EOF == sscanf_s(buf, "%3lu", &len)) {
		onTransferError("Could not parse filename header size!");
		return -1;
	}

	// Finally, allocate a string and receive the filename
	char *nameBuf = (char*)malloc(len + 1);
	nameBuf[len] = '\0';
	bytes_received = s.recv(nameBuf, len, 0);
	if (bytes_received == SOCKET_ERROR || bytes_received != len) {
		free(nameBuf);
		onTransferError("Could not receive filename header!");
	}

	filename = string(nameBuf);
	free(nameBuf);
	return 0;
}

int FileTransferProtocol::sendCommand(net::Socket &s, char command) {
	int bytes_sent = s.send(&command, 1, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != 1) {
		onTransferError("Could not send command!");
		return -1;
	}
	return 0;
}

int FileTransferProtocol::recvCommand(net::Socket &s, char &command) {
	char d;
	int bytes_received;
	bytes_received = s.recv(&d, 1, 0);
	if (bytes_received == SOCKET_ERROR || bytes_received != 1) {
		onTransferError("Could not receive direction from client!");
		return -1;
	}
	if (d != 'D' && d != 'U' && d != 'L' && d != 'Q') {
		onTransferError("Client sent invalid direction!");
		return -1;
	}
	command = d;
	return 0;
}

int FileTransferProtocol::sendAck(net::Socket &s) {
	char buf[3] = { 'A', 'C', 'K' };
	int bytes_sent = s.send(buf, 3, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != 3) {
		onTransferError("Could not acknowledge client!");
		return -1;
	}
	return 0;
}
int FileTransferProtocol::sendErr(net::Socket &s) {
	char buf[3] = { 'E', 'R', 'R' };
	int bytes_sent = s.send(buf, 3, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != 3) {
		onTransferError("Could not send error notice to client!");
		return -1;
	}
	return 0;
}

int FileTransferProtocol::waitForAck(net::Socket &s, string errmsg) {
	char ackbuf[4] = "   "; // Either ACK or ERR
	int bytes_received = s.recv(ackbuf, 3, 0);
	if (bytes_received == SOCKET_ERROR || bytes_received != 3 || strcmp(ackbuf, "ERR") == 0) {
		onTransferError(errmsg);
		return -1;
	}
	return 0;
}
