#include "FileTransferProtocolClient.h"
#include "StreamSocketSender.h"
#include "StreamSocketReceiver.h"
#include <WinSock2.h>
#include "PhilSock.h"
#include <iostream>

FileTransferProtocolClient::FileTransferProtocolClient(const sockaddr_in * name, int namelen) :
		socket(AF_INET, 0, TRACE, name, namelen)
{
}


FileTransferProtocolClient::~FileTransferProtocolClient()
{
}

int FileTransferProtocolClient::list()
{
	// First, send LIST command to server
	if (-1 == sendCommand(socket, 'L')) return -1;

	// Wait for confirmation
	if (-1 == waitForAck(socket, "Server cannot list files at this time.")) return -1;

	while(true) {
		string filename;
		if (-1 == recvFilename(socket, filename)) {
			onTransferError("Could not receive next filename in server list!");
			return -1;
		}
		if (filename.compare("!ENDOFLIST!") == 0) break;

		cout << "+ " << filename << endl;
	}
	cout << "Got ENDOFLIST marker." << endl;
	return 0;
}

int FileTransferProtocolClient::send(string filename, istream &file, size_t file_size) {

	// First, inform server that we are sending a file
	if (-1 == sendCommand(socket, 'U')) return -1;

	// Wait for confirmation
	if (-1 == waitForAck(socket, "Server cannot receive upload at this time.")) return -1;

	// Then, send the filename that the server must write to
	if (-1 == sendFilename(socket, filename)) return -1;

	// Wait for confirmation
	if (-1 == waitForAck(socket, "Server did not accept filename.")) return -1;

	onTransferBegin('U', filename);

	//Finally, send the file.
	StreamSocketSender sender(128); //Send chunks of 128 bytes
	sender.send(socket, file, file_size); // XXX catch exceptions and report progress as it goes along

	onTransferComplete(filename);
	return 0;
}

int FileTransferProtocolClient::receive(string filename, ostream &file) {

	// First, inform server that we are receiving a file
	if (-1 == sendCommand(socket, 'D')) return -1;

	// Wait for confirmation
	if (-1 == waitForAck(socket, "Server cannot send a file at this time.")) return -1;

	// Then, send the requested filename
	if (-1 == sendFilename(socket, filename)) return -1;

	// Wait for confirmation
	if (-1 == waitForAck(socket, "Error: this file does not exist or is unreadable.")) return -1;

	onTransferBegin('D', filename);

	// Finally, receive the file.
	StreamSocketReceiver receiver(128);
	receiver.receive(socket, file); // XXX catch exceptions and report progress as it goes along

	onTransferComplete(filename);

	return 0;
}

int FileTransferProtocolClient::quit()
{
	if (-1 == sendCommand(socket, 'Q')) return -1;
	return 0;
}

void FileTransferProtocolClient::onTransferBegin(char direction, string filename) {
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