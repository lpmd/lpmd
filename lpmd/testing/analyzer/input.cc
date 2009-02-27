//
//
//

#include "input.h"

using namespace lpmd;

//
//
//
AnalyzerInputReader::AnalyzerInputReader(PluginManager & pm, Map & params): CommonInputReader(pm, params) 
{ 
 DeclareStatement("property", "module start end each");
}

