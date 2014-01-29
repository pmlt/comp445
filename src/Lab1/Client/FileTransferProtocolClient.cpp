#include "FileTransferProtocolClient.h"
#include "StreamSocketSender.h"
#include "StreamSocketReceiver.h"
#include <WinSock2.h>

FileTransferProtocolClient::FileTransferProtocolClient()
{
}


FileTransferProtocolClient::~FileTransferProtocolClient()
{
}

int FileTransferProtocolClient::send(SOCKET s, string filename, istream file, size_t file_size) {

	// First, inform server that we are sending a file
	if (-1 == sendDirection(s, 'U')) return -1;

	// Wait for confirmation
	if (-1 == waitForAck(s, "Server cannot receive upload at this time.")) return -1;

	// Then, send the filename that the server must write to
	if (-1 == sendFilename(s, filename)) return -1;

	// Wait for confirmation
	if (-1 == waitForAck(s, "Server did not accept filename.")) return -1;

	onTransferBegin(filename);

	//Finally, send the file.
	StreamSocketSender sender(128); //Send chunks of 128 bytes
	sender.send(s, file, file_size); // XXX catch exceptions and report progress as it goes along

	onTransferComplete(filename);
	return 0;
}

int FileTransferProtocolClient::receive(SOCKET s, string filename, ostream file) {

	// First, inform server that we are receiving a file
	if (-1 == sendDirection(s, 'D')) return -1;

	// Wait for confirmation
	if (-1 == waitForAck(s, "Server cannot send a file at this time.")) return -1;

	// Then, send the requested filename
	if (-1 == sendFilename(s, filename)) return -1;

	// Wait for confirmation
	if (-1 == waitForAck(s, "Error: this file does not exist or is unreadable.")) return -1;

	onTransferBegin(filename);

	// Finally, receive the file.
	StreamSocketReceiver receiver(128);
	receiver.receive(s, file); // XXX catch exceptions and report progress as it goes along

	onTransferComplete(filename);

	return 0;
}

int FileTransferProtocolClient::sendDirection(SOCKET s, char direction) {
	int bytes_sent = ::send(s, &direction, 1, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != 1) {
		onTransferError("Could not send direction header!");
		return -1;
	}
	return 0;
}

int FileTransferProtocolClient::sendFilename(SOCKET s, string filename) {
	int bytes_sent;
	char fNameHeader[3];
	sprintf_s(fNameHeader, "%03lu", (unsigned long)filename.size());
	bytes_sent = ::send(s, fNameHeader, sizeof fNameHeader, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != sizeof fNameHeader) {
		onTransferError("Could not send filename size");
		return -1;
	}
	bytes_sent = ::send(s, filename.c_str(), filename.size(), 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != filename.size()) {
		onTransferError("Could not send filename");
		return -1;
	}
	return 0;
}

int FileTransferProtocolClient::waitForAck(SOCKET s, string errmsg) {
	char ackbuf[4] = "   "; // Either ACK or ERR
	int bytes_received = recv(s, ackbuf, sizeof ackbuf, 0);
	if (bytes_received == SOCKET_ERROR || bytes_received != sizeof ackbuf || strcmp(ackbuf, "ERR") == 0) {
		onTransferError(errmsg);
		return -1;
	}
	return 0;
}

void FileTransferProtocolClient::onTransferBegin(string filename) {

}
void FileTransferProtocolClient::onTransferProgress(string filename, size_t bytes_transferred, size_t total_bytes) {

}
void FileTransferProtocolClient::onTransferComplete(string filename) {

}
void FileTransferProtocolClient::onTransferError(string error) {

}