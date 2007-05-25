class Tasks
{
public:
	typedef std::vector<UnknownField>       UnknownsType;
	uint8_t RecType;
    uint32_t RecordId;
    
    uint8_t TaskType;
    std::string Summary;
    std::string Notes;
    std::string Categories;
    std::string UID;
    
    time_t StartTime;
    time_t DueTime;
    time_t AlarmTime;
    int TimeZoneCode;
    
    enum AlarmFlagType
    {
    	Date = 1,
    	Relative
    };
    AlarmFlagType AlarmType;
    
    unsigned short Interval;
    enum RecurringCodeType {
		Day = 1,		//< eg. every day
					//< set: nothing
		MonthByDate = 3,	//< eg. every month on the 12th
					//< set: DayOfMonth
		MonthByDay = 4,		//< eg. every month on 3rd Wed
					//< set: DayOfWeek and WeekOfMonth
		YearByDate = 5,		//< eg. every year on March 5
					//< set: DayOfMonth and MonthOfYear
		YearByDay = 6,		//< eg. every year on 3rd Wed of Jan
					//< set: DayOfWeek, WeekOfMonth, and
					//<      MonthOfYear
		Week = 12		//< eg. every week on Mon and Fri
					//< set: WeekDays
	};
    RecurringCodeType RecurringType;
	time_t RecurringEndTime;	
		unsigned short			// recurring details, depending on type
		DayOfWeek,		// 0-6
		WeekOfMonth,		// 1-5
		DayOfMonth,		// 1-31
		MonthOfYear;		// 1-12
	unsigned char WeekDays;		// bitmask, bit 0 = sunday
    
    int ClassType;
    enum PriorityFlagType
    {
    	High = 0,
    	Normal,
    	Low
    };
    PriorityFlagType PriorityFlag;
    
    enum StatusFlagType
    {
    	NotStarted = 0,
    	InProgress,
    	Completed,
    	Waiting,
    	Deferred
    };
    StatusFlagType StatusFlag;
    
    bool Recurring;
    bool Perpetual;
    bool DueDateFlag;	// true if due date is set

    UnknownsType Unknowns;

public:	
	Tasks();
	~Tasks();
	
    const unsigned char* ParseField(const unsigned char *begin,
        const unsigned char *end);	
	void ParseRecurrenceData(const void *data);
	void BuildRecurrenceData(void *data);
	uint8_t GetRecType() const { return RecType; }
	uint32_t GetUniqueId() const { return RecordId; }
	void SetIds(uint8_t Type, uint32_t Id) { RecType = Type; RecordId = Id; }
	void ParseHeader(const Data &data, size_t &offset);
	void ParseFields(const Data &data, size_t &offset);
	void BuildHeader(Data &data, size_t &offset) const;

	void Clear();

	void Dump(std::ostream &os) const;

	// database name
	static const char * GetDBName() { return "Tasks"; }
	static uint8_t GetDefaultRecType() { return 2; }
	
};
inline std::ostream& operator<<(std::ostream &os, const Tasks &msg) {
	msg.Dump(os);
	return os;
}
