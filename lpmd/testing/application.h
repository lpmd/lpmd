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

   void FillAtomsFromCellReader();
   void RunModifiers();
   void RunVisualizers();
   void SaveCurrentConfiguration();

   virtual void ProcessControl(int argc, const char * argv[]);
   virtual void CheckConsistency();
   virtual void ConstructCell();
   virtual void ConstructSimulation();
   virtual void FillAtoms();
   virtual void AdjustAtomProperties();
   virtual void OpenOutputStreams();
   virtual void CloseOutputStreams();
   virtual void ApplyPrepares();
   virtual void ApplyFilters();
   virtual void SetPotentials();
   virtual void Iterate();

   virtual int Run();

 protected:
   const std::string name;
   const std::string cmdname;
   PluginManager pluginmanager;
   UtilityControl & innercontrol;
   Simulation * simulation;
   NonOrthogonalCell cell;
   Array<std::ostream *> propertystream; 
   std::ofstream ** outputstream;
   std::istream * inputfile_stream;
};

#endif

