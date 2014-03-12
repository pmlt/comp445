#include "FileTransferProtocol.h"
#include "PhilSock.h"

FileTransferProtocol::FileTransferProtocol()
{
}


FileTransferProtocol::~FileTransferProtocol()
{
}

int FileTransferProtocol::sendFilename(SOCKET s, string filename) {
	int bytes_sent;
	char fNameHeader[4];
	sprintf_s(fNameHeader, "%03lu", (unsigned long)filename.size());
	bytes_sent = net::send(s, fNameHeader, 3, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != 3) {
		onTransferError("Could not send filename size");
		return -1;
	}
	bytes_sent = net::send(s, filename.c_str(), filename.size(), 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != filename.size()) {
		onTransferError("Could not send filename");
		return -1;
	}
	return 0;
}

int FileTransferProtocol::recvFilename(SOCKET s, string &filename) {
	int bytes_received;

	// First, receive the filename size (3 bytes ascii-encoded unsigned integer)
	char buf[3];
	bytes_received = net::recv(s, buf, 3, 0);
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
	bytes_received = net::recv(s, nameBuf, len, 0);
	if (bytes_received == SOCKET_ERROR || bytes_received != len) {
		free(nameBuf);
		onTransferError("Could not receive filename header!");
	}

	filename = string(nameBuf);
	free(nameBuf);
	return 0;
}

int FileTransferProtocol::sendCommand(SOCKET s, char command) {
	int bytes_sent = net::send(s, &command, 1, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != 1) {
		onTransferError("Could not send command!");
		return -1;
	}
	return 0;
}

int FileTransferProtocol::recvCommand(SOCKET s, char &command) {
	char d;
	int bytes_received;
	bytes_received = net::recv(s, &d, 1, 0);
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

int FileTransferProtocol::sendAck(SOCKET s) {
	char buf[3] = { 'A', 'C', 'K' };
	int bytes_sent = net::send(s, buf, 3, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != 3) {
		onTransferError("Could not acknowledge client!");
		return -1;
	}
	return 0;
}
int FileTransferProtocol::sendErr(SOCKET s) {
	char buf[3] = { 'E', 'R', 'R' };
	int bytes_sent = net::send(s, buf, 3, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != 3) {
		onTransferError("Could not send error notice to client!");
		return -1;
	}
	return 0;
}

int FileTransferProtocol::waitForAck(SOCKET s, string errmsg) {
	char ackbuf[4] = "   "; // Either ACK or ERR
	int bytes_received = net::recv(s, ackbuf, 3, 0);
	if (bytes_received == SOCKET_ERROR || bytes_received != 3 || strcmp(ackbuf, "ERR") == 0) {
		onTransferError(errmsg);
		return -1;
	}
	return 0;
}
