
#include "parser.h"
#include "protocol.h"

bool Parser::GetOperation(const Data &data, unsigned int &operation)
{
	// check size to make sure we have up to the DBAccess operation byte
	if( (unsigned int)data.GetSize() < (SB_PACKET_DBACCESS_HEADER_SIZE + 1) )
		return false;

	MAKE_PACKET(pack, data);
	operation = pack->data.db.data.db.operation;
	return true;
}

