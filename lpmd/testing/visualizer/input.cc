//
//
//

#include "input.h"

using namespace lpmd;

//
//
//
VisualizerInputReader::VisualizerInputReader(PluginManager & pm): CommonInputReader(pm) 
{ 
 DeclareStatement("visualize", "module start end each");
}

