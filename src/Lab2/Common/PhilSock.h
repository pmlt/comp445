#pragma once
#include <WinSock2.h>

#define SEQNO_MIN   0
#define SEQNO_MAX   255
#define SEQNO_MASK  0x01 // This is how we define how many bits are in a sequence number.
#define NET_TIMEOUT 300 // in milliseconds

namespace net {
	enum MSGTYPE {
		INIT,   // Sent by client to initiate a connection
		SYN,    // Server response to INIT
		SYNACK, // Client response to SYN
		ACK,    // Receiver response to a DATA packet
		DATA    // Datagram containing application data
	};

	struct state {
		::SOCKET winsocket;
	};

	// Our datagram structure, with headers
	struct dgram {
		int seqno;        // Sequence number
		MSGTYPE type;     // The type of message
		size_t size;      // How large is the payload (in bytes)
		void * payload;   // Pointer to beginning of payload
	};
	
	SOCKET socket(
		int af,
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

	/********* INTERNAL METHODS *************/

	int sendPacket(dgram &d);
	int recvPacket(dgram &d);

	int genSeqNo();
	int nextSeqNo(int seqNo);

	// Constructor for a INIT message
	void init(dgram &d);

	// Constructor for a SYN message
	void syn(dgram &d);

	// Constructor for a SYNACK message
	void synack(dgram &d, int seqNo);

	// Constructor for a ACK message
	void ack(dgram &d, int seqNo);

	// Constructor for a DATA message
	void data(dgram &d, int seqNo, size_t sz, void * buf);
}