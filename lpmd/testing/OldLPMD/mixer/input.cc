//
//
//

#include "input.h"

using namespace lpmd;

//
//
//
MixerInputReader::MixerInputReader(PluginManager & pm, Map & params): CommonInputReader(pm, params) 
{ 
 DeclareStatement("apply", "module");
}

