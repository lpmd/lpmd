//
//
//

#ifndef __MYLPMD_CMDLINE_H__
#define __MYLPMD_CMDLINE_H__

#include <lpmd/cmdline.h>

class LPMDCmdLineParser: public lpmd::CmdLineParser
{
 public:
  //
  LPMDCmdLineParser(int argc, char *argv[]);
  ~LPMDCmdLineParser();

};

#endif


