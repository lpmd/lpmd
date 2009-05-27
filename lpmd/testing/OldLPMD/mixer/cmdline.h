//
//
//

#ifndef __LPMDMIXER_CMDLINE_H__
#define __LPMDMIXER_CMDLINE_H__

#include "../common/cmdline.h"

class MixerCmdLineParser: public CommonCmdLineParser
{
 public:
  //
  MixerCmdLineParser(int argc, char *argv[]): CommonCmdLineParser(argc, argv) { }
  ~MixerCmdLineParser() { }
};

#endif

