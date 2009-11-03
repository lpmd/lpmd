/*
 *
 *
 *
 */

#include "visualizer.h"
#include "quickmode.h"
#include "config.h"
#include "replayintegrator.h"

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
 if (innercontrol["filter-end"] == "false") ApplyFilters();
 if (innercontrol.Defined("cellmanager-module"))
 {
  simulation->SetCellManager(CastModule<CellManager>(pluginmanager[innercontrol["cellmanager-module"]]));
 }
 //
 Iterate();
 //
 return 0;
}

void Visualizer::Iterate()
{
 Plugin & inputplugin = pluginmanager["input1"];
 ReplayIntegrator * replay = 0;
 if (inputfile_stream != 0)
 {
  replay = new ReplayIntegrator(inputplugin, *inputfile_stream);
  simulation->SetIntegrator(*replay);
 }
 while (true)
 {
  if (bool(control["verbose"])) simulation->ShowInfo(std::cout);
  try
  {
   simulation->GetCellManager().UpdateCell(*simulation);
  }
  catch (Error & e) { } // FIXME: Chequear de mejor manera la presencia del cellmanager 
  UpdateAtomicIndices();
  if (innercontrol["filter-end"] == "true")
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
  try { simulation->DoStep(); }
  catch (RuntimeError & rt) { break; }
 }
 delete replay;
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

