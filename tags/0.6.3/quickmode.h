/*
 *
 *
 *
 */

#ifndef __LPMDUTIL_QUICKMODE_H__
#define __LPMDUTIL_QUICKMODE_H__

#include <lpmd/cmdline.h>

using namespace lpmd;

class QuickModeParser: public CommandArguments
{
 public:
   QuickModeParser(const std::string & use_hint);
   ~QuickModeParser();
   std::string FormattedAsControlFile() const;

 private:
   class QuickModeImpl * impl;
};

#endif

