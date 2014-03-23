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

	// Our datagram header structure
	struct dgram {
		int seqno;        // Sequence number
		int ack_seqno;    // For ACK and SYNACK messages; SeqNo of ACKed message
		MSGTYPE type;     // The type of message
		size_t size;      // How large is the payload (in bytes)
	};

	class Socket {
	protected:
		::SOCKET winsocket;
		int af;
		int protocol;

		int this_seqno;
		int dest_seqno;

		dgram lastAck;

		bool trace;
		std::ofstream tracefile;

		int genSeqNo();
		int nextSeqNo(int seqNo);

		// Constructor for a SYN message
		void syn(dgram &d);

		// Constructor for a SYNACK message
		void synack(dgram &d, dgram acked);

		// Constructor for a ACK message
		void ack(dgram &d, int seqNo, dgram acked);

		// Constructor for a DATA message
		void data(dgram * d, int seqNo, size_t sz);

	public:
		sockaddr_in dest;
		int dest_len;

		Socket(int af, int protocol, bool trace);
		virtual ~Socket();

		// This will BLOCK until we have received ACK
		int send(const char * buf, int len, int flags);

		// This will BLOCK until we have received expected #
		int recv(char * buf, int len, int flags);

		// Convenience methods
		int recv_dgram(dgram &pkt, size_t payload_size, void *payload, bool acceptDest = 0);
		int send_dgram(const dgram &pkt);
		int send_ack(int seqNo, dgram acked);
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