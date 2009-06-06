/*
 *
 *
 *
 */

#ifndef __LPMDUTIL_CONVERTER_H__
#define __LPMDUTIL_CONVERTER_H__

#include "application.h"
#include <iostream>

class Converter: public Application
{
 public:
   Converter(int argc, const char * argv[]);

   void FillAtoms();
   void Iterate();

 private:
   UtilityControl control; 
};

#endif

