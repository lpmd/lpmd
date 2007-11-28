//
//
//

#include <lpmd/util.h>
#include "cmdline.h"

LPMDCmdLineParser::LPMDCmdLineParser(int argc, char *argv[])
{
 DefineOption("help", "h", "");
 DefineOption("systemfile", "s", "file");
 DefineOption("version", "v", "");
 Parse(argc, argv);
 if (! Defined("systemfile")) AssignParameter("help", "true");
}

LPMDCmdLineParser::~LPMDCmdLineParser() { }


