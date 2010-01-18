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
#include "replayintegrator.h"

using namespace lpmd;

class Application
{
 public:
   Application(const std::string & appname, const std::string & cmdname, UtilityControl & uc);
   virtual ~Application();

   void PrintBanner(const std::string & text);
   void ShowPluginHelp();
   void ShowHelp();
   void AutoTestPlugin(const std::string & pluginname);

   void FillAtomsFromCellReader();
   void PreReadConfigurations();
   void UpdateAtomicIndices();
   void ShowApplicableModules(const std::string & kind);
   void RunModifiers();
   void RunVisualizers();
   void SaveCurrentConfiguration();

   virtual void ProcessControl(int argc, const char * argv[], const std::string & use_hint);
   virtual void CheckConsistency();
   virtual void ConstructCell();
   virtual void ConstructSimulation();
   virtual void FillAtoms();
   virtual void OptimizeSimulationAtStart();
   virtual void AdjustAtomProperties();
   virtual void ComputeProperties();
   virtual void OpenOutputStreams();
   virtual void CloseOutputStreams();
   virtual void OpenPropertyStreams();
   virtual void ClosePropertyStreams();
   virtual void ApplyPrepares();
   virtual void ApplyFilters();
   virtual void SetPotentials();
   virtual void Iterate();

   virtual int Run();

 protected:
   const std::string name;
   const std::string cmdname;
   std::string * indexbuffer;
   long int old_atoms_size;
   PluginManager pluginmanager;
   UtilityControl & innercontrol;
   Simulation * simulation;
   NonOrthogonalCell cell;
   Array<std::ostream *> propertystream; 
   std::ofstream ** outputstream;
   std::istream * inputfile_stream;
   ReplayIntegrator * replay;
};

#endif

