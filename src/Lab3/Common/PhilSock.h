#pragma once
#include <WinSock2.h>
#include <fstream>

#define TRACEFILE   "trace.log"
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

	// Our datagram structure
	struct dgram {
		int seqno;        // Sequence number
		MSGTYPE type;     // The type of message
		size_t size;      // How large is the payload (in bytes)
		char payload[MAX_PAYLOAD_SIZE];
	};

	class Socket {
	protected:
		// WINSOCK variables
		::SOCKET winsocket;
		int af;
		int protocol;

		// SENDER state
		int sndr_seqno;
		const char * sndr_buf; // Pointer to the next byte to send
		size_t sndr_len; // How many bytes left to send

		// RECEIVER state
		int recv_seqno;
		char * recv_buf; // Pointer to the location where next received byte will be written
		size_t recv_len; // How many bytes left we are expecting to receive

		// DEBUG variables
		bool trace;
		std::ofstream tracefile;

		int genSeqNo();
		int nextSeqNo(int seqNo);

		// Constructor for a SYN message
		void syn(dgram &d);

		// Constructor for a SYNACK message
		void synack(dgram &d, dgram acked);

		// Constructor for a ACK message
		void ack(dgram &d, dgram acked);

		// Constructor for a DATA message
		void data(dgram &d, int seqNo, size_t sz, const char * payload);

		// Generic blocking method. Blocks until all outstanding operations are completed (both send and receive)
		void wait(size_t &recv, size_t &sent);

		int sendNextPacket();

	public:
		sockaddr_in dest;
		int dest_len;

		Socket(int af, int protocol, bool trace);
		virtual ~Socket();

		// Reliable (blocking) send method
		size_t send(const char * buf, int len);

		// Reliable (blocking) recv method
		size_t recv(char * buf, int len);

		// Convenience methods
		int recv_dgram(dgram &pkt, bool acceptDest = 0);
		int send_dgram(const dgram &pkt);
		int send_ack(dgram acked);
		void traceError(int err, char *msg);
	};

	class ClientSocket : public Socket {
	private:
		sockaddr_in local_addr;
	public:
		// Automatic connect on object creation
		ClientSocket(int af, int protocol, bool trace, int local_port, const struct sockaddr_in * name, int namelen);
	};

	class ServerSocket : public Socket {
	public:
		// Automatic bind on object creation
		ServerSocket(int af, int protocol, bool trace, const struct sockaddr_in * name, int namelen);

		void listen(int backlog);
	};

	class SocketException : std::exception {
	public:
		SocketException(const char * msg) : exception(msg) {}
	};
}