class Memos
{
public:
    typedef std::vector<UnknownField>       UnknownsType;

    uint8_t RecType;
    uint32_t RecordId;

	uint8_t MemoType;
    std::string Title;
    std::string Body;
    std::string Category;
    
    UnknownsType Unknowns;
    
public:
    const unsigned char* ParseField(const unsigned char *begin,
        const unsigned char *end);
public:
    Memos();
    ~Memos();
    
        // Parser / Builder API (see parser.h / builder.h)
    uint8_t GetRecType() const { return RecType; }
    uint32_t GetUniqueId() const { return RecordId; }
    void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
    void ParseHeader(const Data &data, size_t &offset);
    void ParseFields(const Data &data, size_t &offset);
    void BuildHeader(Data &data, size_t &offset) const;

    void Clear();

    void Dump(std::ostream &os) const;

    // database name
    static const char * GetDBName() { return "Memos"; }
    static uint8_t GetDefaultRecType() { return 0; }    // or 0?
};
inline std::ostream& operator<<(std::ostream &os, const Memos &msg) {
    msg.Dump(os);
    return os;
}

