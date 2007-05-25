///////////////////////////////////////////////////////////////////////////////
// Memo Class

// Memo Field Codes
#define MEMFC_TITLE     0x01
#define MEMFC_BODY      0x02
#define MEMFC_MEMO_TYPE	0x03
#define MEMFC_CATEGORY  0x04
#define MEMFC_END       0xffff

FieldLink<Memos> MemosFieldLinks[] = {
    { MEMFC_TITLE,     "Title",     0, 0,	&Memos::Title, 0, 0 },
    { MEMFC_BODY,      "Body",      0, 0,	&Memos::Body, 0, 0 },
    { MEMFC_CATEGORY,  "Category",  0, 0,	&Memos::Category, 0, 0 },
    { MEMFC_END,	"End of List",	0, 0,	0, 0, 0 }
};

Memos::Memos()
{
    Clear();
}

Memos::~Memos()
{
}

const unsigned char* Memos::ParseField(const unsigned char *begin,
                      const unsigned char *end)
{
    const CommonField *field = (const CommonField *) begin;

    // advance and check size
    begin += COMMON_FIELD_HEADER_SIZE + btohs(field->size);
    if( begin > end )       // if begin==end, we are ok
        return begin;

    if( !btohs(field->size) )   // if field has no size, something's up
        return begin;

	if( field->type == MEMFC_MEMO_TYPE ) {
		if( ( MemoType = field->u.raw[0] ) != 'm' ) {
			throw Error( "Memos::ParseField: MemoType is not 'm'" );
		}
		return begin;
	}
			
				 
    // cycle through the type table
    for(    FieldLink<Memos> *b = MemosFieldLinks;
        b->type != MEMFC_END;
        b++ )
    {
        if( b->type == field->type ) {
            if( b->strMember ) {
                std::string &s = this->*(b->strMember);
                s.assign((const char *)field->u.raw, btohs(field->size)-1);
                return begin;   // done!
            }
            else if( b->timeMember && btohs(field->size) == 4 ) {
                time_t &t = this->*(b->timeMember);
                t = min2time(field->u.min1900);
                return begin;
            }
        }
    }
    // if still not handled, add to the Unknowns list
    UnknownField uf;
    uf.type = field->type;
    uf.data.assign((const char*)field->u.raw, btohs(field->size));
    Unknowns.push_back(uf);

    // return new pointer for next field
    return begin;
}

void Memos::ParseHeader(const Data &data, size_t &offset)
{
    // no header in Memos records
}

void Memos::ParseFields(const Data &data, size_t &offset)
{
    const unsigned char *finish = ParseCommonFields(*this,
        data.GetData() + offset, data.GetData() + data.GetSize());
    offset += finish - (data.GetData() + offset);
}


void Memos::Dump(std::ostream &os) const
{
	os << "Memos entry: 0x" << setbase(16) << RecordId
		<< " (" << (unsigned int)RecType << ")\n";
    os << "    Title: " << Title << "\n";
    os << "    Body: " << Body << "\n";
    os << "    Category: " << Category << "\n";
    
    os << Unknowns;
    os << "\n\n";
}

void Memos::Clear()
{
    Title.clear();
    Body.clear();
    Category.clear();
    
    MemoType = 0;
    
    Unknowns.clear();
}

