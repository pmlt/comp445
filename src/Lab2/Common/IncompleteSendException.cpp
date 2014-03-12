#include "IncompleteSendException.h"


IncompleteSendException::IncompleteSendException(size_t bytes_sent, size_t expected_bytes_sent)
{
	this->bytes_sent = bytes_sent;
	this->expected_bytes_sent = expected_bytes_sent;
}


IncompleteSendException::~IncompleteSendException()
{
}
