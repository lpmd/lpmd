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
   Application(const std::string & appname, const std::string & cmdname, UtilityControl & uc);
   ~Application();

   void PrintBanner(const std::string & text);
   void ShowPluginHelp();
   void ShowHelp();

   virtual void CheckConsistency();
   virtual void ConstructCell();
   virtual void FillAtoms();
   virtual void ApplyPrepares();
   virtual void SetPotentials();
   virtual void Iterate();

   int Run();

 protected:
   const std::string name;
   const std::string cmdname;
   PluginManager pluginmanager;
   UtilityControl & innercontrol;
   Simulation * simulation;
   NonOrthogonalCell cell;
};

#endif

