//
//
//

#include "input.h"

using namespace lpmd;

//
//
//
ConverterInputReader::ConverterInputReader(PluginManager & pm, Map & params): CommonInputReader(pm, params) 
{ 
 DeclareStatement("apply", "module start end each");
}

