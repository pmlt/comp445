#include "PhilSock.h"
#include <WinSock2.h>

SOCKET net::socket(int af, int protocol) {
	SOCKET winsocket = ::socket(af, SOCK_DGRAM, protocol);
	if (winsocket == INVALID_SOCKET) return winsocket;

	// Set the recv timeout
	DWORD timeout = NET_TIMEOUT;
	setsockopt(winsocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

	state *ptr = (state*)malloc(sizeof(state));
	ptr->winsocket = winsocket;
	ptr->af = af;
	ptr->protocol = protocol;
	
	return (SOCKET)ptr;
}

int net::connect(SOCKET s, const struct sockaddr * name, int namelen) {
	// First, initialize state
	state *st = (state*)s;
	memcpy(&st->dest, name, sizeof(sockaddr));
	st->dest_len = namelen;

	// Three-way handshake.
	// Step 1: Send the sequence number to the server
	dgram _syn;
	syn(_syn);
	st->this_seqno = _syn.seqno;
	sendto(st->winsocket, (const char*)&_syn, sizeof(_syn), 0, name, namelen);

	// Step 2: Wait for SYNACK
	while (true) {
		dgram _synack;
		int recvbytes = recvfrom(st->winsocket, (char*)&_synack, sizeof(_synack), 0, 0, 0);
		if (recvbytes == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
			// Some other error occured
			return SOCKET_ERROR;
		}
		if (recvbytes != sizeof(_synack)) continue; // Throw away unexpected packet
		if (_synack.type != SYNACK) continue; //Throw away unexpected packet

		st->this_seqno = nextSeqNo(st->this_seqno);
		st->dest_seqno = nextSeqNo(_synack.seqno);

		// Step 3: Send ACK
		dgram _ack;
		ack(_ack, _synack);
		sendto(st->winsocket, (const char*)&_ack, sizeof(_ack), 0, name, namelen);
		return 0;
	}
}

int net::bind(SOCKET s, const struct sockaddr * name, int namelen) {
	SOCKET winsocket = getwinsocket(s);
	return ::bind(winsocket, name, namelen);
}

int net::listen(SOCKET s, int backlog) {
	// First, initialize state
	state *st = (state*)s;
	memset(&st->dest, 0, sizeof(st->dest));
	st->dest_len = sizeof(st->dest);
	memset(&st->incoming_syn, 0, sizeof(st->incoming_syn));

	// Wait for a SYN
	dgram _syn;
	while (true) {
		int recvbytes = recvfrom(st->winsocket, (char*)&_syn, sizeof(_syn), 0, &st->dest, &st->dest_len);
		if (recvbytes == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
			// Some other error occured
			return SOCKET_ERROR;
		}
		if (recvbytes != sizeof(_syn)) continue; // Throw away unexpected packet
		if (_syn.type != SYN) continue; //Throw away unexpected packet

		break;
	}
	st->incoming_syn = _syn;
	st->dest_seqno = _syn.seqno;
	// Here we return, and let the user accept the connection
	return 0;
}

SOCKET net::accept(SOCKET s, struct sockaddr * addr, int * addrlen) {
	// Complete the connection handshake.
	state *st = (state*)s;

	// Send a SYNACK
	dgram _synack;
	synack(_synack, st->incoming_syn);
	sendto(st->winsocket, (const char*)&_synack, sizeof(_synack), 0, &st->dest, st->dest_len);

	// Wait for ACK
	dgram _ack;
	while (true) {
		int recvbytes = recvfrom(st->winsocket, (char*)&_ack, sizeof(_ack), 0, 0, 0);
		if (recvbytes == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
			// Some other error occured
			return SOCKET_ERROR;
		}
		if (recvbytes != sizeof(_ack)) continue; // Throw away unexpected packet
		if (_ack.type != ACK) continue; //Throw away unexpected packet
		break;
	}

	state *c_st = (state*)malloc(sizeof(state));
	c_st->af = st->af;
	c_st->protocol = st->protocol;
	c_st->dest = st->dest;
	c_st->dest_len = st->dest_len;
	c_st->dest_seqno = nextSeqNo(st->dest_seqno);
	c_st->winsocket = st->winsocket;
	c_st->this_seqno = nextSeqNo(_synack.seqno);
	memset(&c_st->incoming_syn, 0, sizeof(c_st->incoming_syn));

	addr = &c_st->dest;
	addrlen = &c_st->dest_len;

	return (SOCKET)c_st;
}

int net::send(SOCKET s, const char * buf, int len, int flags) {
	state *st = (state*)s;
	int bytes_sent = 0;
	while (len > 0) {
		// Each iteration of this loop equals to a packet being sent

		// First, prepare packet
		size_t pl_size = min(MAX_PAYLOAD_SIZE, len);
		size_t pkt_size = sizeof(dgram)+pl_size;
		char * packet = (char*)malloc(pkt_size);
		// Initialize headers
		data((dgram&)packet, st->this_seqno, pl_size, (void*)buf);
		//Copy payload data
		memcpy(packet + sizeof(dgram), buf, pl_size);

		// The packet is ready to be sent.
		while (true) {
			//Each iteration of this loop equals to an attempt to send the packet.

			// Send the packet
			sendto(st->winsocket, packet, sizeof(dgram) + pl_size, 0, &st->dest, st->dest_len);

			// Wait for ACK
			dgram _ack;
			int recvbytes = recvfrom(st->winsocket, (char*)&_ack, sizeof(_ack), 0, NULL, NULL);
			if (recvbytes == SOCKET_ERROR) {
				if (WSAGetLastError() == WSAETIMEDOUT) continue; // Timed out, try again
				// Some other error occured
				free(packet); // Don't leak
				return SOCKET_ERROR;
			}

			//Check that this is indeed an ACK
			if (_ack.type != ACK) continue;

			//Check that this is the expected sequence number
			if (_ack.seqno != st->dest_seqno) continue;

			// Packet was acknowledged!
			break;
		}
		// Packet is sent; increment sequence numbers, free memory and move on to next.
		st->this_seqno = nextSeqNo(st->this_seqno);
		st->dest_seqno = nextSeqNo(st->dest_seqno);
		free(packet);
		buf += pl_size;
		len = max(0, len - pl_size);
		bytes_sent += pl_size;
	}
	// Sent all payload bytes!
	return bytes_sent;
}

int net::recv(SOCKET s, char * buf, int len, int flags) {
	state *st = (state*)s;

	int bytes_received = 0;
	while (len > 0) {
		// Each iteration of this loop equals to a packet being received
		size_t pl_size = min(MAX_PAYLOAD_SIZE, len);
		size_t pkt_size = sizeof(dgram)+pl_size;
		char *pkt = (char*)malloc(pkt_size);

		while (true) {

			// Each iteration of this loop equals to an attempt to receive a packet.
			int recvbytes = recvfrom(st->winsocket, pkt, pkt_size, 0, NULL, NULL);
			if (recvbytes == SOCKET_ERROR) {
				free(pkt);
				return SOCKET_ERROR;
			}
			if (recvbytes < pkt_size) continue; //I don't know WHAT that is...

			dgram *pkt_header = (dgram*)pkt;

			// Check that packet is a DATA packet
			if (pkt_header->type != DATA) continue;

			// Check that it is of the correct sequence number
			if (pkt_header->seqno != st->dest_seqno) continue;
			
			break;
		}
		// This is our packet! Copy into output buffer, increment sequence numbers, free memory, etc.
		st->this_seqno = nextSeqNo(st->this_seqno);
		st->dest_seqno = nextSeqNo(st->dest_seqno);
		memcpy(buf, pkt + sizeof(dgram), pl_size);
		buf += pl_size;
		len = max(0, len - pl_size);
		free(pkt);
		bytes_received += pl_size;
	}
	return bytes_received;
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

int net::genSeqNo() {
	return (rand() / (SEQNO_MAX - SEQNO_MIN)) + SEQNO_MIN;
}

int net::nextSeqNo(int seqNo) {
	return (seqNo + 1) & SEQNO_MASK;
}

void net::syn(dgram &d) {
	d.seqno = genSeqNo();
	d.type = SYN;
	d.size = 0;
	d.payload = NULL;
}

void net::synack(dgram &d, dgram prevsyn) {
	d.seqno = genSeqNo();
	d.type = SYNACK;
	d.size = 0;
	d.payload = NULL;
}

void net::ack(dgram &d, dgram prev) {
	d.seqno = prev.seqno;
	d.type = ACK;
	d.size = 0;
	d.payload = NULL;
}

void net::data(dgram &d, int seqNo, size_t sz, void * buf) {
	d.seqno = seqNo;
	d.type = DATA;
	d.size = sz;
	d.payload = buf;
}