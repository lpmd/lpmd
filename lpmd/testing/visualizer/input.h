//
//
//

#ifndef __LPMDVISUALIZER_INPUT_H__
#define __LPMDVISUALIZER_INPUT_H__

#include "../common/input.h"

class VisualizerInputReader: public CommonInputReader
{
 public:
   VisualizerInputReader(lpmd::PluginManager & pm);
};

#endif

