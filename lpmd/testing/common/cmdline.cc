//
//
//

#include "cmdline.h"

#include <lpmd/util.h>
#include <iostream>

CommonCmdLineParser::CommonCmdLineParser(int argc, char *argv[])
{
 DefineOption("help", "h", "");
 DefineOption("verbose", "v", "");
 DefineOption("pluginhelp", "p", "file");
 DefineOption("option", "O", "keywordvalue");
 DefineOption("lengths", "L", "a,b,c");
 DefineOption("angles", "A", "alpha,beta,gamma");
 DefineOption("vector", "V", "ax,ay,az,bx,by,bz,cx,cy,cz");
 DefineOption("scale", "S", "value");
 DefineOption("input", "i", "options");
 DefineOption("output", "o", "options");
 DefineOption("use", "u", "options");
 DefineOption("cellmanager", "c", "options");
 DefineOption("replace-cell", "r", "");
 Parse(argc, argv);
}

CommonCmdLineParser::~CommonCmdLineParser() { }

