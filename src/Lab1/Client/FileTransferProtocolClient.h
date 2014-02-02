#pragma once

#include <string>
#include <WinSock2.h>
#include <iostream>
#include "FileTransferProtocol.h"

using namespace std;

class FileTransferProtocolClient : FileTransferProtocol
{
public:
	FileTransferProtocolClient();
	~FileTransferProtocolClient();

	SOCKET connect(HOSTENT &remote);

	int list(SOCKET s);
	int send(SOCKET s, string filename, istream &file, size_t file_size);
	int receive(SOCKET s, string filename, ostream &file);
	int quit(SOCKET s);

	virtual void onTransferBegin(char direction, string filename);
	virtual void onTransferProgress(string filename, size_t bytes_transferred, size_t total_bytes);
	virtual void onTransferComplete(string filename);
	virtual void onTransferError(string error);
};

