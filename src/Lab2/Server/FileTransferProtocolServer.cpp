#include "FileTransferProtocolServer.h"
#include "StreamSocketReceiver.h"
#include "StreamSocketSender.h"
#include <malloc.h>
#include <sys/stat.h>
#include <fstream>
#include <windows.h>

using namespace std;

FileTransferProtocolServer::FileTransferProtocolServer(string docroot)
{
	this->docroot = docroot;
}


FileTransferProtocolServer::~FileTransferProtocolServer()
{
}

int FileTransferProtocolServer::serve(SOCKET s) {
	char command;

	while (true) {
		// First, wait for direction
		if (-1 == recvCommand(s, command)) return -1;

		// OK! At this point, the next action depends on the command received.
		if (command == 'D') {
			if (-1 == serveDownload(s)) return -1;
		}
		else if (command == 'U') {
			if (-1 == serveUpload(s)) return -1;
		}
		else if (command == 'L') {
			if (-1 == serveList(s)) return -1;
		}
		else if (command == 'Q') {
			return 0;
		}
	}
	return 0;
}

int FileTransferProtocolServer::serveList(SOCKET s) {
	WIN32_FIND_DATA ffd;
	HANDLE hFind;

	hFind = FindFirstFile((this->docroot + "/*").c_str(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		onTransferError("Could not list directory contents.");
		sendErr(s);
		return -1;
	}
	// Send ack
	if (-1 == sendAck(s)) return -1;
	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			// Only list files
			if (-1 == sendFilename(s, ffd.cFileName)) {
				FindClose(hFind);
				return -1;
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);
	FindClose(hFind);

	// Inform client that listing is done; 
	// since ! is not a valid filename character, there cannot be a 
	// conflict with the actual directory listing
	if (-1 == sendFilename(s, "!ENDOFLIST!")) return -1;
	return 0;
}

int FileTransferProtocolServer::serveUpload(SOCKET s) {
	string filename;

	// Send ack
	if (-1 == sendAck(s)) return -1;

	// Second, wait for filename
	if (-1 == recvFilename(s, filename)) return -1;
	string filepath = this->docroot + "/" + filename;

	// Receive the file from client.
	ofstream file(filepath.c_str(), ios::out | ios::binary | ios::trunc);
	if (!file) {
		onTransferError("Could not open file for writing!");
		if (-1 == sendErr(s)) return -1;
		return -1;
	}
	// Ready to receive file, informing client...
	if (-1 == sendAck(s)) return -1;
	onTransferBegin('U', filename);
	StreamSocketReceiver receiver(128); //Receive chunks of 128 bytes
	receiver.receive(s, file); // XXX catch exceptions and report progress as it goes along
	onTransferComplete(filename);
	return 0;
}

int FileTransferProtocolServer::serveDownload(SOCKET s) {
	string filename;

	// Send ack
	if (-1 == sendAck(s)) return -1;

	// Second, wait for filename
	if (-1 == recvFilename(s, filename)) return -1;
	string filepath = this->docroot + "/" + filename;

	// At this point, we should check if file exists
	struct stat stats;
	int statres = stat(filepath.c_str(), &stats);
	if (statres == -1) {
		char err[255];
		strerror_s(err, 255, errno);
		onTransferError(err);
		if (-1 == sendErr(s)) return -1;
		return 0;
	}
	// Open file for reading
	ifstream file(filepath.c_str(), ios::in | ios::binary);
	if (!file) {
		onTransferError("Could not open file for reading!");
		if (-1 == sendErr(s)) return -1;
		return -1;
	}
	// Ready to send file, informing client...
	if (-1 == sendAck(s)) return -1;
	onTransferBegin('D', filename);
	StreamSocketSender sender(128); //Send chunks of 128 bytes
	sender.send(s, file, stats.st_size); // XXX catch exceptions and report progress as it goes along
	onTransferComplete(filename);
	return 0;
}

void FileTransferProtocolServer::onTransferBegin(char direction, string filename)
{
	if (direction == 'U') {
		cout << "Receiving " << filename << " from client..." << endl;
	}
	else {
		cout << "Sending " << filename << " to client..." << endl;
	}
}

void FileTransferProtocolServer::onTransferProgress(string filename, size_t bytes_transferred, size_t total_bytes)
{
	cout << filename << ": " << bytes_transferred << " of " << total_bytes << "bytes (" << (int)((bytes_transferred / total_bytes) * 100) << "%)" << endl;
}
void FileTransferProtocolServer::onTransferComplete(string filename)
{
	cout << filename << " transferred!" << endl;
}
void FileTransferProtocolServer::onTransferError(string error)
{
	cout << "An error occured during transfer: " << error << endl;
}