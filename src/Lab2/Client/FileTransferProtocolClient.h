#pragma once

#include <string>
#include <iostream>
#include "PhilSock.h"
#include "FileTransferProtocol.h"

#define TRACE 1

using namespace std;

class FileTransferProtocolClient : FileTransferProtocol
{
private:
	net::ClientSocket socket;

public:
	FileTransferProtocolClient(int local_port, const sockaddr_in * name, int namelen);
	~FileTransferProtocolClient();

	int list();
	int send(string filename, istream &file, size_t file_size);
	int receive(string filename, ostream &file);
	int quit();

	virtual void onTransferBegin(char direction, string filename);
	virtual void onTransferProgress(string filename, size_t bytes_transferred, size_t total_bytes);
	virtual void onTransferComplete(string filename);
	virtual void onTransferError(string error);
};

