//
//
//

#ifndef __LPMDVISUALIZER_CMDLINE_H__
#define __LPMDVISUALIZER_CMDLINE_H__

#include "../common/cmdline.h"

class VisualizerCmdLineParser: public CommonCmdLineParser
{
 public:
  //
  VisualizerCmdLineParser(int argc, char *argv[]): CommonCmdLineParser(argc, argv) { }
  ~VisualizerCmdLineParser() { }

};

#endif


