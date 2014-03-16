#pragma once
#include <WinSock2.h>

#define MAX_PAYLOAD_SIZE 1000 // Maximum payload size (in bytes)
#define SEQNO_MIN   0
#define SEQNO_MAX   255
#define SEQNO_MASK  0x01 // This is how we define how many bits are in a sequence number.
#define NET_TIMEOUT 300  // Time to wait for ACK in milliseconds

namespace net {
	enum MSGTYPE {
		SYN,    // Client sends this to initialize connection
		SYNACK, // Server response to SYN
		ACK,    // Acknowledgement
		DATA    // Datagram containing application data
	};

	// Our datagram structure, with headers
	struct dgram {
		int seqno;        // Sequence number
		MSGTYPE type;     // The type of message
		size_t size;      // How large is the payload (in bytes)
		void * payload;   // Pointer to beginning of payload
	};

	struct state {
		::SOCKET winsocket;
		int af;
		int protocol;

		int this_seqno;
		int dest_seqno;
		sockaddr dest;
		int dest_len;
		dgram incoming_syn;
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

	// This will BLOCK until we have received ACK
	int send(
		SOCKET s,
		const char * buf,
		int len,
		int flags);
	
	// This will BLOCK until we have received expected #
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

	// Constructor for a SYN message
	void syn(dgram &d);

	// Constructor for a SYNACK message
	void synack(dgram &d, dgram prevsyn);

	// Constructor for a ACK message
	void ack(dgram &d, dgram prev);

	// Constructor for a DATA message
	void data(dgram * d, int seqNo, size_t sz, void * buf);
}