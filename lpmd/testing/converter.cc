/*
 *
 *
 *
 */

#include "converter.h"
#include "quickmode.h"
#include "config.h"
#include "replayintegrator.h"

#include <fstream>
#include <lpmd/cellformat.h>
#include <lpmd/session.h>
#include <lpmd/error.h>

int main(int argc, const char * argv[])
{
 try
 {
  Converter main(argc, argv);
  return main.Run();
 }
 catch (std::exception & e)
 {
  std::cerr << "[Error] " << e.what() << '\n';
  return 1;
 }
}

void Converter::FillAtoms() { FillAtomsFromCellReader(); }

void Converter::Iterate()
{
 ReplayIntegrator * replay = 0;
 Plugin & inputplugin = pluginmanager["input1"];
 if (inputfile_stream != 0)
 {
  replay = new ReplayIntegrator(inputplugin, *inputfile_stream);
  simulation->SetIntegrator(*replay);
 }
 while (true)
 {
  if (bool(control["verbose"])) std::cerr << "\n-> Processing configuration " << simulation->CurrentStep() << '\n';
  else std::cerr << "-> Processing configuration " << simulation->CurrentStep() << "                     \r";
  std::cerr.flush();
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
  RunVisualizers();
  SaveCurrentConfiguration();
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
 delete replay;
}

Converter::Converter(int argc, const char * argv[]): Application("LPMD Converter", "lpmd-converter", control), control(pluginmanager)
{
 std::cerr << "\nLPMD Converter, version " << VERSION << "\n\n";
 ProcessControl(argc, argv, "apply");
}

Converter::~Converter()
{
 std::cerr << "\n-> Done.\n";
}
