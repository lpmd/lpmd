//
//
//

#ifndef __LPMDCONVERTER_CMDLINE_H__
#define __LPMDCONVERTER_CMDLINE_H__

#include "../common/cmdline.h"

class ConverterCmdLineParser: public CommonCmdLineParser
{
 public:
  //
  ConverterCmdLineParser(int argc, char *argv[]): CommonCmdLineParser(argc, argv) { }
  ~ConverterCmdLineParser() { }

};

#endif


