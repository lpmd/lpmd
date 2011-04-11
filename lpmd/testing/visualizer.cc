/*
 *
 *
 *
 */

#include "visualizer.h"
#include "quickmode.h"
#include "config.h"
#include "replayintegrator.h"
#include "refsimulation.h"

#include <fstream>
#include <unistd.h>
#include <lpmd/cellformat.h>
#include <lpmd/session.h>
#include <lpmd/error.h>

int main(int argc, const char * argv[])
{
 try
 {
  Visualizer main(argc, argv);
  return main.Run();
 }
 catch (std::exception & e)
 {
  std::cerr << "[Error] " << e.what() << '\n';
  return 1;
 }
}

void Visualizer::FillAtoms() { FillAtomsFromCellReader(); }

int Visualizer::Run()
{
 CheckConsistency();
 // 
 ConstructCell();
 ConstructSimulation();
 FillAtoms();
 AdjustAtomProperties();
 SetPotentials();
 ApplyPrepares();
 // if (innercontrol["filter-at-end"] == "false") ApplyFilters();
 if (innercontrol.Defined("cellmanager-module"))
 {
  simulation->SetCellManager(CastModule<CellManager>(pluginmanager[innercontrol["cellmanager-module"]]));
 }
 //
 Iterate();
 //
 return 0;
}

void Visualizer::IterateSequential()
{
 while (true)
 {
  if (bool(control["verbose"])) simulation->ShowInfo(std::cout);
  try
  {
   simulation->GetCellManager().UpdateCell(*simulation);
  }
  catch (Error & e) { } // FIXME: Chequear de mejor manera la presencia del cellmanager 
  UpdateAtomicIndices();
  if (innercontrol["filter-at-end"] == "true")
  {
   RunModifiers();
   ApplyFilters();
  } 
  else
  {
   ApplyFilters();
   RunModifiers();
  }
  if (control.Defined("delay")) usleep(long(double(control["delay"])*1000000.0));
  RunVisualizers();
  if (inputfile_stream == 0) break;
  try 
  { 
   while (1)
   {
    try { simulation->DoStep(); break; }
    catch (InvalidRequest & ir) { }
   }
  }
  catch (RuntimeError & rt) { break; }
 }
}

void Visualizer::IterateReplay()
{
 std::cerr << "-> Replay mode\n";
 simulation->SetIntegrator(*replay);
 SimulationHistory & sh = replay->History();
 Simulation * old_simulation = simulation;
 bool inLoop = true;
 while (inLoop)
 {
  for (long i=0;i<sh.Size();++i)
  {
   std::cerr << "-> Playing configuration " << i << " of " << sh.Size() << '\n';
   try
   {
    simulation = new RefSimulation(sh[i]);
    simulation->AdjustCurrentStep(i);
    if (bool(control["verbose"])) simulation->ShowInfo(std::cout);
    try
    {
     simulation->GetCellManager().UpdateCell(*simulation);
    }
    catch (Error & e) { } // FIXME: Chequear de mejor manera la presencia del cellmanager 
    UpdateAtomicIndices();
    if (innercontrol["filter-at-end"] == "true")
    {
     RunModifiers();
     ApplyFilters();
    } 
    else
    {
     ApplyFilters();
     RunModifiers();
    }
    if (control.Defined("delay")) usleep(long(double(control["delay"])*1000000.0));
    RunVisualizers();
    if (simulation != 0) delete simulation;
   }
   catch (RuntimeError & rt) { inLoop = false; break; }
  }
  if (! bool(innercontrol["loop"])) break;
 }
 delete old_simulation;
}

void Visualizer::Iterate()
{
 if (innercontrol.Defined("replay") && (innercontrol["replay"] == "true")) IterateReplay();
 else
 {
  if (inputfile_stream != 0)
  {
   Plugin & inputplugin = pluginmanager["input1"];
   replay = new ReplayIntegrator(inputplugin, *inputfile_stream);
   simulation->SetIntegrator(*replay);
  }
  IterateSequential();
 }
}

Visualizer::Visualizer(int argc, const char * argv[]): Application("LPMD Visualizer", "lpmd-visualizer", control), control(pluginmanager)
{
 std::cerr << "\nLPMD Visualizer, version " << VERSION << "\n\n";
 ProcessControl(argc, argv, "visualize");
}

Visualizer::~Visualizer()
{
 std::cerr << "-> Done.\n";
}

