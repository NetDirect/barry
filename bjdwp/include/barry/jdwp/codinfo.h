#ifndef __BARRYJDG_CODINFO_H__
#define __BARRYJDG_CODINFO_H__


#include <string>
#include <fstream>
#include <vector>


#define COD_DEBUG_APPNAME_HEADERFIELD	0x0
#define COD_DEBUG_UNIQUEID_HEADERFIELD	0x8

#define COD_DEBUG_NONE_FIELD			0x0
#define COD_DEBUG_BOOLEAN_FIELD			0x1
#define COD_DEBUG_BYTE_FIELD			0x2
#define COD_DEBUG_CHAR_FIELD			0x3
#define COD_DEBUG_SHORT_FIELD			0x4
#define COD_DEBUG_INT_FIELD				0x5
#define COD_DEBUG_LONG_FIELD			0x6
#define COD_DEBUG_CLASS_FIELD			0x7
#define COD_DEBUG_ARRAY_FIELD			0x8
#define COD_DEBUG_VOID_FIELD			0xA
#define COD_DEBUG_DOUBLE_FIELD			0xC


namespace Barry {

namespace JDG {


class JDGDebugFileList;
class JDGDebugFileEntry;
class JDGClassList;
class JDGClassEntry;


class JDGDebugFileList : public std::vector<JDGDebugFileEntry> {
public:
	void AddElement(uint32_t uniqueid, std::string appname, std::string filename);
	void Dump(std::ostream &os) const;	
};
inline std::ostream& operator<<(std::ostream &os, const JDGDebugFileList &list) {
	list.Dump(os);
	return os;
}


class JDGDebugFileEntry {
protected:

public:
	std::string fileName;
	std::string appName;
	uint32_t uniqueId;

	void Dump(std::ostream &os) const;

private:
};


class JDGClassList : public std::vector<JDGClassEntry> {
protected:

public:
	void createDefaultEntries();

private:
};


class JDGClassEntry {
protected:

public:
	// For JDB
	int index;

	// Read from the ".debug" file
	std::string className;
	std::string classPath;
	std::string sourceFile;

	uint32_t type;
	uint32_t unknown02;
	uint32_t unknown03;
	uint32_t id;
	uint32_t unknown05;
	uint32_t unknown06;
	uint32_t unknown07;
	uint32_t unknown08;

	std::string getFullClassName() { return classPath + "." + className; };

private:
};




class JDGCodInfo {
protected:

public:
	uint32_t uniqueId;
	std::string appName;
	JDGClassList classList;

	bool loadDebugFile(const char *filename);

	void parseHeaderSection(std::ifstream &input);
	void parseTypeSection(std::ifstream &input);

	uint32_t getUniqueId();
	std::string getAppName();

private:
	uint32_t parseNextHeaderField(std::ifstream &input);
	uint32_t parseNextTypeField(std::ifstream &input);

	void parseAppName(std::ifstream &input);
	void parseUniqueId(std::ifstream &input);

	void parseBoolean(std::ifstream &input);
	void parseByte(std::ifstream &input);
	void parseChar(std::ifstream &input);
	void parseShort(std::ifstream &input);
	void parseInt(std::ifstream &input);
	void parseLong(std::ifstream &input);
	void parseClass(std::ifstream &input);
	void parseArray(std::ifstream &input);
	void parseVoid(std::ifstream &input);
	void parseDouble(std::ifstream &input);
};


void searchDebugFile(JDGDebugFileList &list);
bool loadDebugInfo(JDGDebugFileList &list, const char *filename, JDGCodInfo &info);
bool loadDebugInfo(JDGDebugFileList &list, const uint32_t uniqueId, const std::string module, JDGCodInfo &info);


} // namespace JDG

} // namespace Barry


#endif

