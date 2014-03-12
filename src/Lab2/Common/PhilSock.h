#pragma once
#include <WinSock2.h>

namespace net {
	struct state {
		::SOCKET winsocket;
	};
	
	SOCKET socket(
		int af,
		int type,
		int protocol);

	int connect(
		SOCKET s,
		const struct sockaddr * name,
		int namelen);
	
	int bind(
		SOCKET s,
		const struct sockaddr * name,
		int namelen);
	
	int listen(
		SOCKET s,
		int backlog);

	SOCKET accept(
		SOCKET s,
		struct sockaddr * addr,
		int * addrlen);

	int send(
		SOCKET s,
		const char * buf,
		int len,
		int flags);
	
	int recv(
		SOCKET s,
		char * buf,
		int len,
		int flags);

	int closesocket(
		SOCKET s);

	SOCKET getwinsocket(SOCKET s);
}