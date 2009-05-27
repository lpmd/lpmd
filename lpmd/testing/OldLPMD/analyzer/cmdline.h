//
//
//

#ifndef __LPMDANALYZER_CMDLINE_H__
#define __LPMDANALYZER_CMDLINE_H__

#include "../common/cmdline.h"

class AnalyzerCmdLineParser: public CommonCmdLineParser
{
 public:
  //
  AnalyzerCmdLineParser(int argc, char *argv[]): CommonCmdLineParser(argc, argv) { }
  ~AnalyzerCmdLineParser() { }
};

#endif


