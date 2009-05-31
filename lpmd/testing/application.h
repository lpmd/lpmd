/*
 *
 *
 *
 */

#ifndef __LPMDUTIL_APPLICATION_H__
#define __LPMDUTIL_APPLICATION_H__

#include <lpmd/pluginmanager.h>
#include <lpmd/simulation.h>
#include <lpmd/nonorthogonalcell.h>
#include "controlparser.h"

using namespace lpmd;

class Application
{
 public:
   Application(const std::string & appname, UtilityControl & uc);
   ~Application();

   virtual void CheckConsistency();
   virtual void ConstructCell();
   virtual void FillAtoms();
   virtual void ApplyPrepares();
   virtual void SetPotentials();
   virtual void Iterate();

   int Run();

 protected:
   PluginManager pluginmanager;
   UtilityControl & innercontrol;
   Simulation * simulation;
   NonOrthogonalCell cell;
};

#endif

