/*
 *
 *
 *
 */

#include "converter.h"
#include "quickmode.h"
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
 bool reading = true;
 ReplayIntegrator replay;
 simulation->SetIntegrator(replay);
 while (reading)
 {
  ApplyPrepares();
  ApplyFilters();
  RunModifiers();
  RunVisualizers();
  SaveCurrentConfiguration();
  if (inputfile_stream == 0) break;
  // 
  Module & inputmodule = pluginmanager["input1"];
  CellReader & cellreader = CastModule<CellReader>(inputmodule);
  simulation->DoStep();
  simulation->Atoms().Clear();
  reading = cellreader.ReadCell(*inputfile_stream, *simulation);
 }
}

Converter::Converter(int argc, const char * argv[]): Application("LPMD Converter", "lpmd-converter", control), control(pluginmanager)
{
 inputfile_stream = 0;
 ProcessControl(argc, argv);
}

Converter::~Converter()
{
 if (inputfile_stream != 0) delete inputfile_stream;
}

