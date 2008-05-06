//
//
//

#include "input.h"

using namespace lpmd;

//
//
//
AnalyzerInputReader::AnalyzerInputReader(PluginManager & pm): CommonInputReader(pm) 
{ 
 DeclareStatement("property", "module start end each");
}

