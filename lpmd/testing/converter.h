/*
 *
 *
 *
 */

#ifndef __LPMDUTIL_CONVERTER_H__
#define __LPMDUTIL_CONVERTER_H__

#include "application.h"
#include <iostream>

class ConverterControl: public UtilityControl
{
 public:
   ConverterControl(PluginManager & pm): UtilityControl(pm) { }


};

class Converter: public Application
{
 public:
   Converter(int argc, const char * argv[]);
   ~Converter();

   void FillAtoms();
   void Iterate();

 private:
   ConverterControl control; 
};

#endif

