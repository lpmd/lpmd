/*
 *
 *
 *
 */

#include "visualizer.h"
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
 ApplyFilters();
 //
 Iterate();
 //
 return 0;
}

void Visualizer::Iterate()
{
 bool reading = true;
 ReplayIntegrator replay;
 simulation->SetIntegrator(replay);
 while (reading)
 {
  ApplyPrepares();
  ApplyFilters();
  RunVisualizers();
  if (inputfile_stream == 0) break;
  // 
  Module & inputmodule = pluginmanager["input1"];
  CellReader & cellreader = CastModule<CellReader>(inputmodule);
  simulation->DoStep();
  simulation->Atoms().Clear();
  reading = cellreader.ReadCell(*inputfile_stream, *simulation);
 }
}

Visualizer::Visualizer(int argc, const char * argv[]): Application("LPMD Visualizer", "lpmd-visualizer", control), control(pluginmanager)
{
 inputfile_stream = 0;
 ProcessControl(argc, argv);
}

Visualizer::~Visualizer()
{
 if (inputfile_stream != 0) delete inputfile_stream;
}

