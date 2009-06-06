/*
 *
 *
 *
 */

#ifndef __LPMDUTIL_ANALYZER_H__
#define __LPMDUTIL_ANALYZER_H__

#include "application.h"
#include <iostream>

class Analyzer: public Application
{
 public:
   Analyzer(int argc, const char * argv[]);

   int Run();
   void Iterate();

 private:
   UtilityControl control; 
};

#endif

