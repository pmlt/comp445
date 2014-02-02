#include "FileTransferProtocolClient.h"
#include "StreamSocketSender.h"
#include "StreamSocketReceiver.h"
#include <WinSock2.h>
#include <iostream>

//user defined port number
#define REQUEST_PORT 0x7070;

FileTransferProtocolClient::FileTransferProtocolClient()
{
}


FileTransferProtocolClient::~FileTransferProtocolClient()
{
}

SOCKET FileTransferProtocolClient::connect(HOSTENT &remote)
{
	SOCKET s;
	SOCKADDR_IN sa;         // filled by bind
	SOCKADDR_IN sa_in;      // fill with server info, IP, port
	int port = REQUEST_PORT;

	//Create the socket
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) return INVALID_SOCKET;
	/* For UDP protocol replace SOCK_STREAM with SOCK_DGRAM */

	//Specify server address for client to connect to server.
	memset(&sa_in, 0, sizeof(sa_in));
	memcpy(&sa_in.sin_addr, remote.h_addr, remote.h_length);
	sa_in.sin_family = remote.h_addrtype;
	sa_in.sin_port = htons(port);

	//Display the host machine internet address

	//cout << "Connecting to remote host:";
	//cout << inet_ntoa(sa_in.sin_addr) << endl;

	//Connect Client to the server
	if (::connect(s, (LPSOCKADDR)&sa_in, sizeof(sa_in)) == SOCKET_ERROR)
		return SOCKET_ERROR;
	
	return s;
}

int FileTransferProtocolClient::send(SOCKET s, string filename, istream &file, size_t file_size) {

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

int FileTransferProtocolClient::receive(SOCKET s, string filename, ostream &file) {

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
	char fNameHeader[4];
	sprintf_s(fNameHeader, "%03lu", (unsigned long)filename.size());
	bytes_sent = ::send(s, fNameHeader, 3, 0);
	if (bytes_sent == SOCKET_ERROR || bytes_sent != 3) {
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
	int bytes_received = recv(s, ackbuf, 3, 0);
	if (bytes_received == SOCKET_ERROR || bytes_received != 3 || strcmp(ackbuf, "ERR") == 0) {
		onTransferError(errmsg);
		return -1;
	}
	return 0;
}

void FileTransferProtocolClient::onTransferBegin(string filename) {
	cout << "Beginning transfer of file " << filename << endl;
}
void FileTransferProtocolClient::onTransferProgress(string filename, size_t bytes_transferred, size_t total_bytes) {
	cout << filename << ": " << bytes_transferred << " of " << total_bytes << "bytes (" << (int)((bytes_transferred / total_bytes) * 100) << "%)" << endl;
}
void FileTransferProtocolClient::onTransferComplete(string filename) {
	cout << filename << " transferred!" << endl;
}
void FileTransferProtocolClient::onTransferError(string error) {
	cout << "An error occured during transfer: " << error << endl;
}