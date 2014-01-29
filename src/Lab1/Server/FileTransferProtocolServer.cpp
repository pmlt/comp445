#include "FileTransferProtocolServer.h"
#include "StreamSocketReceiver.h"
#include "StreamSocketSender.h"
#include <malloc.h>
#include <sys/stat.h>
#include <fstream>

using namespace std;

FileTransferProtocolServer::FileTransferProtocolServer(string docroot)
{
	this->docroot = docroot;
}


FileTransferProtocolServer::~FileTransferProtocolServer()
{
}

int FileTransferProtocolServer::serve(SOCKET s) {
	char direction;
	string filename;

	// First, wait for direction
	if (-1 == recvDirection(s, direction)) return -1;

	// Send ack
	if (-1 == sendAck(s)) return -1;

	// Second, wait for filename
	if (-1 == recvFilename(s, filename)) return -1;
	string filepath = this->docroot + "/" + filename;

	// At this point, we should check if file exists
	struct stat stats;
	int file_exists = stat(filepath.c_str(), &stats) != 0;


	// OK! At this point, the next action depends on the direction.
	if (direction == 'D') {
		if (!file_exists) {
			onTransferError("Client requested invalid file, closing connection...");
			if (-1 == sendErr(s)) return -1;
			return 0;
		}
		// Send the file to the client.
		ifstream file(filepath.c_str(), ios::in | ios::binary);
		if (!file) {
			onTransferError("Could not open file for reading!");
			if (-1 == sendErr(s)) return -1;
			return -1;
		}
		// Ready to send file, informing client...
		if (-1 == sendAck(s)) return -1;
		StreamSocketSender sender(128); //Send chunks of 128 bytes
		sender.send(s, file, stats.st_size); // XXX catch exceptions and report progress as it goes along
	}
	else if (direction == 'U') {
		// Receive the file from client.
		ofstream file(filepath.c_str(), ios::out | ios::binary | ios::trunc);
		if (!file) {
			onTransferError("Could not open file for writing!");
			if (-1 == sendErr(s)) return -1;
			return -1;
		}
		// Ready to receive file, informing client...
		if (-1 == sendAck(s)) return -1;
		StreamSocketReceiver receiver(128); //Receive chunks of 128 bytes
		receiver.receive(s, file); // XXX catch exceptions and report progress as it goes along
	}
	return 0;
}

int FileTransferProtocolServer::recvDirection(SOCKET s, char &direction) {
	char d;
	int bytes_received;
	bytes_received = recv(s, &d, 1, 0);
	if (bytes_received == SOCKET_ERROR || bytes_received != 1) {
		onTransferError("Could not receive direction from client!");
		return -1;
	}
	if (d != 'D' && d != 'U') {
		onTransferError("Client sent invalid direction!");
		return -1;
	}
	direction = d;
	return 0;
}
int FileTransferProtocolServer::recvFilename(SOCKET s, string &filename) {
	int bytes_received;

	// First, receive the filename size (3 bytes ascii-encoded unsigned integer)
	char buf[3];
	bytes_received = recv(s, buf, 20, 0);
	if (bytes_received == SOCKET_ERROR) {
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
	char *nameBuf = (char*)malloc(len);
	bytes_received = recv(s, nameBuf, len, 0);
	if (bytes_received == SOCKET_ERROR || bytes_received != len) {
		free(nameBuf);
		onTransferError("Could not receive filename header!");
	}

	filename.resize(len);
	filename.copy(nameBuf, len, 0);
	free(nameBuf);
	return 0;
}

int FileTransferProtocolServer::sendAck(SOCKET s) {
	char buf[3] = { 'A', 'C', 'K' };
	int bytes_sent = ::send(s, buf, 3, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != 3) {
		onTransferError("Could not acknowledge client!");
		return -1;
	}
	return 0;
}
int FileTransferProtocolServer::sendErr(SOCKET s) {
	char buf[3] = { 'E', 'R', 'R' };
	int bytes_sent = ::send(s, buf, 3, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != 3) {
		onTransferError("Could not send error notice to client!");
		return -1;
	}
	return 0;
}

void FileTransferProtocolServer::onTransferBegin(char direction, string filename)
{

}

void FileTransferProtocolServer::onTransferProgress(string filename, size_t bytes_received, size_t total_bytes)
{

}
void FileTransferProtocolServer::onTransferComplete(string filename)
{

}
void FileTransferProtocolServer::onTransferError(string error)
{

}