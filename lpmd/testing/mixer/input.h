//
//
//

#ifndef __LPMDMIXER_INPUT_H__
#define __LPMDMIXER_INPUT_H__

#include "../common/input.h"

class MixerInputReader: public CommonInputReader
{
 public:
   MixerInputReader(lpmd::PluginManager & pm, lpmd::Map & params);
};

#endif

