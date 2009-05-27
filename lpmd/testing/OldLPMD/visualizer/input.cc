//
//
//

#include "input.h"

using namespace lpmd;

//
//
//
VisualizerInputReader::VisualizerInputReader(PluginManager & pm, Map & params): CommonInputReader(pm, params) 
{ 
 DeclareStatement("visualize", "module start end each");
}

