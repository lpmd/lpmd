//
//
//

#include "input.h"

using namespace lpmd;

//
//
//
ConverterInputReader::ConverterInputReader(PluginManager & pm): CommonInputReader(pm) 
{ 
 DeclareStatement("apply", "module start end each");
}

