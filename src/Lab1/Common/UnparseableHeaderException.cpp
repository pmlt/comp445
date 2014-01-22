#include "UnparseableHeaderException.h"


UnparseableHeaderException::UnparseableHeaderException(char *header)
{
	this->header = header;
}


UnparseableHeaderException::~UnparseableHeaderException()
{
}
