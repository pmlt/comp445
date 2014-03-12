#pragma once
#include <string>
#include <WinSock2.h>
#include "FileTransferProtocol.h"

using namespace std;

class FileTransferProtocolServer : FileTransferProtocol
{
private:
	string docroot;

public:
	FileTransferProtocolServer(string docroot);
	~FileTransferProtocolServer();

	int serve(SOCKET s);
	int serveList(SOCKET s);
	int serveUpload(SOCKET s);
	int serveDownload(SOCKET s);

	virtual void onTransferBegin(char direction, string filename);
	virtual void onTransferProgress(string filename, size_t bytes_received, size_t total_bytes);
	virtual void onTransferComplete(string filename);
	virtual void onTransferError(string error);
};

