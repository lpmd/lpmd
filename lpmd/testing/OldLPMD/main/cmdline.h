//
//
//

#ifndef __LPMDMAIN_CMDLINE_H__
#define __LPMDMAIN_CMDLINE_H__

#include "../common/cmdline.h"

class LPMDCmdLineParser: public CommonCmdLineParser
{
 public:
  //
  LPMDCmdLineParser(int argc, char *argv[]): CommonCmdLineParser(argc, argv) { }
  ~LPMDCmdLineParser() { }
};

#endif


