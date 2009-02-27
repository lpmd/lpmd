//
//
//

#ifndef __LPMDANALYZER_INPUT_H__
#define __LPMDANALYZER_INPUT_H__

#include "../common/input.h"

class ConverterInputReader: public CommonInputReader
{
 public:
   ConverterInputReader(lpmd::PluginManager & pm, lpmd::Map & params);
};

#endif

