#pragma once

#include <string>
#include <WinSock2.h>
#include <iostream>

using namespace std;

class FileTransferProtocolClient
{
public:
	FileTransferProtocolClient();
	~FileTransferProtocolClient();

	int send(SOCKET s, string filename, istream file, size_t file_size);
	int receive(SOCKET s, string filename, ostream file);

	int sendDirection(SOCKET s, char direction);
	int sendFilename(SOCKET s, string filename);
	int waitForAck(SOCKET s, string errmsg);

	virtual void onTransferBegin(string filename);
	virtual void onTransferProgress(string filename, size_t bytes_transferred, size_t total_bytes);
	virtual void onTransferComplete(string filename);
	virtual void onTransferError(string error);
};

