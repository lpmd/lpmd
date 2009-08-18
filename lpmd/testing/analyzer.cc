/*
 *
 *
 *
 */

#include "analyzer.h"
#include "quickmode.h"
#include "config.h"

#include <fstream>
#include <lpmd/value.h>
#include <lpmd/cellformat.h>
#include <lpmd/session.h>
#include <lpmd/error.h>

int main(int argc, const char * argv[])
{
 try
 {
  Analyzer main(argc, argv);
  return main.Run();
 }
 catch (std::exception & e)
 {
  std::cerr << "[Error] " << e.what() << '\n';
  return 1;
 }
}

int Analyzer::Run()
{
 CheckConsistency();
 // 
 ConstructCell();
 ConstructSimulation();
 FillAtomsFromCellReader();
 AdjustAtomProperties();
 SetPotentials();
 ApplyPrepares();
 ApplyFilters();
 if (innercontrol.Defined("cellmanager-module"))
 {
  simulation->SetCellManager(CastModule<CellManager>(pluginmanager[innercontrol["cellmanager-module"]]));
 }
 OpenPropertyStreams();
 //
 CheckForTemporalProperties();
 if (temporalproperties.Size() > 0) PreReadConfigurations();
 //
 Iterate();
 //
 if (temporalproperties.Size() > 0) ComputeTemporalProperties();
 ClosePropertyStreams();
 //
 return 0;
}

void Analyzer::CheckForTemporalProperties()
{
 temporalproperties.Clear();
 Array<std::string> properties = StringSplit(innercontrol["property-modules"]); 
 for (int p=0;p<properties.Size();++p)
 {
  Module & rawmodule = pluginmanager[properties[p]];
  if (pluginmanager.HasType<lpmd::TemporalProperty>(properties[p])) 
     temporalproperties.Append(&(CastModule<lpmd::TemporalProperty>(rawmodule)));
 }
}

void Analyzer::PreReadConfigurations()
{
 assert(inputfile_stream != 0);
 Plugin & inputmodule = pluginmanager["input1"];
 replay = new ReplayIntegrator(inputmodule, *inputfile_stream);
 replay->PreRead(*simulation);
}

void Analyzer::ComputeTemporalProperties()
{
 for (int i=0;i<temporalproperties.Size();++i)
 {
  TemporalProperty & prop = *(temporalproperties[i]);
  Plugin & rawmodule = dynamic_cast<Plugin&>(prop);
  prop.Evaluate(replay->History(), simulation->Potentials());
  lpmd::AbstractValue & value = CastModule<lpmd::AbstractValue>(rawmodule);
  value.OutputToFile(rawmodule["output"]);
 }
}

void Analyzer::Iterate()
{
 Plugin & inputmodule = pluginmanager["input1"];
 if ((replay == 0) && (inputfile_stream != 0)) 
 {
  replay = new ReplayIntegrator(inputmodule, *inputfile_stream);
  simulation->SetIntegrator(*replay);
 }
 else if (replay != 0) { simulation->SetIntegrator(*replay); }
 while (true)
 {
  if (bool(control["verbose"])) std::cerr << "-> Processing configuration " << simulation->CurrentStep() << '\n';
  else std::cerr << "-> Processing configuration " << simulation->CurrentStep() << "                     \r";
  std::cerr.flush();
  if (bool(control["verbose"])) simulation->ShowInfo(std::cout);
  UpdateAtomicIndices();
  ApplyFilters();
  ComputeProperties();
  if (inputfile_stream == 0) break;
  try { simulation->DoStep(); }
  catch (RuntimeError & rt) { break; }
 }
}

Analyzer::Analyzer(int argc, const char * argv[]): Application("LPMD Analyzer", "lpmd-analyzer", control), control(pluginmanager)
{
 replay = 0;
 std::cerr << "\nLPMD Analyzer, version " << VERSION << "\n\n";
 ProcessControl(argc, argv, "property");
}

Analyzer::~Analyzer()
{ 
 std::cerr << "\n-> Done.\n";
 delete replay; 
}

