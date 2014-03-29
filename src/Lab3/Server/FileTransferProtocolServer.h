#pragma once
#include <string>
#include "PhilSock.h"
#include "FileTransferProtocol.h"

#define TRACE 1

using namespace std;

class FileTransferProtocolServer : FileTransferProtocol
{
private:
	string docroot;
	net::ServerSocket socket;

public:
	FileTransferProtocolServer(string docroot, const struct sockaddr_in * name, int namelen);
	~FileTransferProtocolServer();

	void waitForClient();
	int serve();
	int serveList();
	int serveUpload();
	int serveDownload();

	virtual void onTransferBegin(char direction, string filename);
	virtual void onTransferProgress(string filename, size_t bytes_received, size_t total_bytes);
	virtual void onTransferComplete(string filename);
	virtual void onTransferError(string error);
};

