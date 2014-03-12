#include "PhilSock.h"
#include <WinSock2.h>

SOCKET net::socket(int af, int protocol) {
	SOCKET winsocket = ::socket(af, SOCK_STREAM, protocol);
	if (winsocket == INVALID_SOCKET) return winsocket;

	state *ptr = (state*)malloc(sizeof(state));
	ptr->winsocket = winsocket;
	
	return (SOCKET)ptr;
}

int net::connect(SOCKET s, const struct sockaddr * name, int namelen) {
	SOCKET winsocket = getwinsocket(s);
	return ::connect(winsocket, name, namelen);
}

int net::bind(SOCKET s, const struct sockaddr * name, int namelen) {
	SOCKET winsocket = getwinsocket(s);
	return ::bind(winsocket, name, namelen);
}

int net::listen(SOCKET s, int backlog) {
	SOCKET winsocket = getwinsocket(s);
	return ::listen(winsocket, backlog);
}

SOCKET net::accept(SOCKET s, struct sockaddr * addr, int * addrlen) {
	SOCKET winsocket = getwinsocket(s);
	SOCKET client = ::accept(winsocket, addr, addrlen);
	if (winsocket == INVALID_SOCKET) return winsocket;
	state *ptr = (state*)malloc(sizeof(state));
	ptr->winsocket = client;
	return (SOCKET)ptr;
}

int net::send(SOCKET s, const char * buf, int len, int flags) {
	SOCKET winsocket = getwinsocket(s);
	return ::send(winsocket, buf, len, flags);
}

int net::recv(SOCKET s, char * buf, int len, int flags) {
	SOCKET winsocket = getwinsocket(s);
	return ::recv(winsocket, buf, len, flags);
}

int net::closesocket(SOCKET s) {
	SOCKET winsocket = getwinsocket(s);
	int ret = ::closesocket(winsocket);
	free((void*)s);
	return ret;
}

SOCKET net::getwinsocket(SOCKET s) {
	state *ptr = (state*)s;
	return ptr->winsocket;
}