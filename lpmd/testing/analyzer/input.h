//
//
//

#ifndef __LPMDANALYZER_INPUT_H__
#define __LPMDANALYZER_INPUT_H__

#include "../common/input.h"

class AnalyzerInputReader: public CommonInputReader
{
 public:
   AnalyzerInputReader(lpmd::PluginManager & pm, lpmd::Map & params);
};

#endif

