/*
 *
 *
 *
 */

#ifndef __LPMDUTIL_LPMD_H__
#define __LPMDUTIL_LPMD_H__

#include "application.h"
#include "controlparser.h"
#include <lpmd/stepper.h>

#include <iostream>

class LPMDControl: public UtilityControl
{
 public:
   LPMDControl(PluginManager & pm): UtilityControl(pm)
   {
    DeclareStatement("steps", "number");
    DeclareStatement("dumping", "each file");
    DeclareStatement("restore", "file");
    DeclareStatement("monitor", "properties start end each output");
    DeclareStatement("integrator", "module start");
   }

};

class LPMD: public Application
{
 public:
   LPMD(int argc, const char * argv[]);

   void FillAtoms();
   void RestoreSimulation();
   void Iterate();

 private:
   LPMDControl control;
   Stepper dumper;
};

#endif

