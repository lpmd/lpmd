//
//
//

#ifndef __LPMDCOMMON_CMDLINE_H__
#define __LPMDCOMMON_CMDLINE_H__

#include <lpmd/cmdline.h>

class CommonCmdLineParser: public lpmd::CmdLineParser
{
 public:
  //
  CommonCmdLineParser(int argc, char *argv[]);
  ~CommonCmdLineParser();

};

#endif

