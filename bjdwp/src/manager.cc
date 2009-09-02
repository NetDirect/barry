#include <iostream>

#include "manager.h"


using namespace std;
using namespace Barry;
using namespace JDG;


namespace JDWP {


// JDWAppInfo class
//------------------


void JDWAppInfo::load(JDGCodInfo &info)
{
	cout << "JDWAppInfo::load" << endl;

	// Assign uniqueId
	uniqueId = info.getUniqueId();

	// Add Class (concat with a previous list)
	JDGClassList *list = &(info.classList);

	classList.insert(classList.end(), list->begin(), list->end());
}


} // namespace JDWP

