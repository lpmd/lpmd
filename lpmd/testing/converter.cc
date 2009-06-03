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

void Converter::FillAtoms()
{
 Module & inputmodule = pluginmanager["input1"];
 if (bool(innercontrol["replacecell"])) inputmodule["replacecell"] = "true";
 else inputmodule["replacecell"] = "false";
 try
 {
  CellReader & cellreader = CastModule<CellReader>(inputmodule);
  GlobalSession.DebugStream() << "-> Reading input file: " << inputmodule["file"] << '\n';
  inputfile_stream = new std::ifstream(inputmodule["file"].c_str());
  cellreader.ReadHeader(*inputfile_stream);
  cellreader.ReadCell(*inputfile_stream, *simulation);
  //
 }
 catch (Error & e)
 {
  //
  CellGenerator & generator = CastModule<CellGenerator>(inputmodule);
  generator.Generate(*simulation);
 }
 if (innercontrol.Defined("cellmanager-module"))
    simulation->SetCellManager(CastModule<CellManager>(pluginmanager[innercontrol["cellmanager-module"]]));
}

void Converter::Iterate()
{
 bool reading = true;
 ReplayIntegrator replay;
 simulation->SetIntegrator(replay);
 while (reading)
 {
  ApplyPrepares();
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

