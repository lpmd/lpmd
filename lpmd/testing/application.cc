/*
 *
 *
 *
 */

#include "application.h"
#include "quickmode.h"
#include <lpmd/simulationbuilder.h>
#include <lpmd/cellgenerator.h>
#include <lpmd/cellreader.h>
#include <lpmd/cellwriter.h>
#include <lpmd/systemmodifier.h>
#include <lpmd/systemfilter.h>
#include <lpmd/visualizer.h>
#include <lpmd/combinedpotential.h>
#include <lpmd/properties.h>
#include <lpmd/property.h>
#include <lpmd/storedvalue.h>
#include <lpmd/value.h>
#include <lpmd/session.h>

#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace lpmd;

Application::Application(const std::string & appname, const std::string & cmd, UtilityControl & uc): name(appname), cmdname(cmd), innercontrol(uc)
{
 simulation = 0;
 inputfile_stream = 0;
 GlobalSession.AssignParameter("debug", "none");
 srand48(long(time(NULL)));
}

Application::~Application() 
{ 
 if (inputfile_stream != 0) delete inputfile_stream;
}

int Application::Run()
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
 if (innercontrol.Defined("cellmanager-module"))
 {
  simulation->SetCellManager(CastModule<CellManager>(pluginmanager[innercontrol["cellmanager-module"]]));
 }
 //
 OpenPropertyStreams();
 OpenOutputStreams();
 Iterate();
 CloseOutputStreams();
 ClosePropertyStreams();
 //
 return 0;
}

void Application::ProcessControl(int argc, const char * argv[], const std::string & use_hint)
{
 QuickModeParser quick(use_hint);
 quick.Parse(argc, argv);
 innercontrol["optimize-simulation"] = "true";
 if (quick.Defined("help")) ShowHelp();
 else if (quick.Defined("test-plugin")) AutoTestPlugin(quick["test-plugin-name"]);
 else
 {
  if (quick.Arguments().Size() == 1)
  {
   std::istringstream generatedcontrol(quick.FormattedAsControlFile());
   innercontrol.Read(generatedcontrol, innercontrol, "quickmode"); 
   if (pluginmanager.IsLoaded("help_plugin")) ShowPluginHelp();
   else if (!innercontrol.Defined("cell-type")) ShowHelp();
   std::cerr << "-> Using quick mode (no control file)\n";
  }
  else 
  {
   std::cerr << "-> Reading quickmode flags\n";
   std::istringstream generatedcontrol(quick.FormattedAsControlFile());
   innercontrol.Read(generatedcontrol, innercontrol, "quickmode");
   std::cerr << "-> Using control file: " << quick.Arguments()[1] << '\n';
   innercontrol.Read(quick.Arguments()[1], innercontrol);
  }
 }
 if (quick.Defined("verbose")) 
 {
  GlobalSession.AssignParameter("debug", "stderr");
  innercontrol["verbose"] = "true";
 }
}

void Application::CheckConsistency()
{
 GlobalSession.DebugStream() << '\n' << innercontrol << '\n';
}

void Application::ConstructCell()
{
 std::string celltype = innercontrol["cell-type"];
 if (celltype == "automatic")
 {
  for (int q=0;q<3;++q) cell[q] = identity[q];
 }
 else if (celltype == "cubic")
 {
  double length = double(innercontrol["cell-a"]);
  double scale = double(innercontrol["cell-scale"]);
  for (int q=0;q<3;++q) cell[q] = length*scale*identity[q];
 }
 else if (celltype == "crystal")
 {
  double box[3];
  box[0] = double(innercontrol["cell-a"]);
  box[1] = double(innercontrol["cell-b"]);
  box[2] = double(innercontrol["cell-c"]);
  double scale = double(innercontrol["cell-scale"]);
  for (int q=0;q<3;++q) cell[q] = box[q]*scale*identity[q];
  double alpha=90,beta=90,gamma=90;
  if(innercontrol.Defined("cell-alpha"))
  {
   alpha = double(innercontrol["cell-alpha"]);
  }
  if(innercontrol.Defined("cell-beta"))
  {
   beta = double(innercontrol["cell-beta"]);
  }
  if(innercontrol.Defined("cell-gamma"))
  {
   gamma = double(innercontrol["cell-gamma"]);
  }
  alpha = alpha*M_PI/180.0;
  beta = beta*M_PI/180.0;
  gamma = gamma*M_PI/180.0;
  cell[0][1] = 0.0e0;
  cell[0][2] = 0.0e0;
  cell[1][0] = cell[1][0]*cos(gamma);
  cell[1][1] = cell[1][1]*cos(gamma);
  cell[1][2] = 0.0e0;
  cell[2][0] = cell[2][0]*cos(beta);
  double tmp=(cos(alpha)-cos(gamma)*cos(beta))/sin(gamma);
  cell[2][1] = cell[2][1]*tmp;
  cell[2][2] = cell[2][1]*sqrt(sin(beta)*sin(beta)-tmp*tmp);
 }
 else if (celltype == "vector")
 {
  double ax, ay, az;
  ax = double(innercontrol["cell-ax"]);
  ay = double(innercontrol["cell-ay"]);
  az = double(innercontrol["cell-az"]);
  cell[0] = Vector(ax, ay, az);
  double bx, by, bz;
  bx = double(innercontrol["cell-bx"]);
  by = double(innercontrol["cell-by"]);
  bz = double(innercontrol["cell-bz"]);
  cell[1] = Vector(bx, by, bz);
  double cx, cy, cz;
  cx = double(innercontrol["cell-cx"]);
  cy = double(innercontrol["cell-cy"]);
  cz = double(innercontrol["cell-cz"]);
  cell[2] = Vector(cx, cy, cz);
 }
 else throw RuntimeError("The cell type must be \'cubic\', \'crystal\' or \'vector\' (or you must use the replacecell flag, -r)");
}

void Application::ConstructSimulation()
{
 NonOrthogonalCell temporary_cell(cell[0], cell[1], cell[2]);
 simulation = &(SimulationBuilder::CreateGeneric());
 for (int q=0;q<3;++q) simulation->Cell()[q] = temporary_cell[q];
}

void Application::FillAtoms()
{
 CellGenerator & cg = CastModule<CellGenerator>(pluginmanager["input1"]);
 if (name == "LPMD") 
 {
  PrintBanner("INPUT MODULES"); 
  pluginmanager["input1"].Show(std::cout);
 }
 cg.Generate(*simulation);
 if (bool(innercontrol["optimize-simulation"])) OptimizeSimulationAtStart();
 else GlobalSession.DebugStream() << "-> NOT optimizing simulation, by user request\n";
}

void Application::FillAtomsFromCellReader()
{
 Module & inputmodule = pluginmanager["input1"];
 if (bool(innercontrol["replacecell"])) inputmodule["replacecell"] = "true";
 else inputmodule["replacecell"] = "false";
 if (pluginmanager.HasType<CellReader>("input1"))
 {
  CellReader & cellreader = CastModule<CellReader>(inputmodule);
  GlobalSession.DebugStream() << "-> Reading input file: " << inputmodule["file"] << '\n';
  inputfile_stream = new std::ifstream(inputmodule["file"].c_str());
  cellreader.ReadHeader(*inputfile_stream);
  cellreader.ReadCell(*inputfile_stream, *simulation);
 }
 else
 {
  CellGenerator & generator = CastModule<CellGenerator>(inputmodule);
  generator.Generate(*simulation);
 }
 if (bool(innercontrol["optimize-simulation"])) OptimizeSimulationAtStart();
 else GlobalSession.DebugStream() << "-> NOT optimizing simulation, by user request\n";
}

void Application::OptimizeSimulationAtStart()
{
 GlobalSession.DebugStream() << "-> Optimizing simulation before starting...\n";
 simulation = &(SimulationBuilder::CloneOptimized(*simulation));
}

void Application::UpdateAtomicIndices()
{
 Tag indextag("index");
 BasicParticleSet & atoms = simulation->Atoms();
 for (long int i=0;i<atoms.Size();++i) atoms.SetTag(atoms[i], indextag, i);
}

void Application::AdjustAtomProperties()
{
 if (simulation == 0) throw InvalidOperation("Adjusting atomic properties on an uninitialized Simulation");
 BasicParticleSet & atoms = simulation->Atoms();
 // FIXME: por ahora solo se considera atom-group y charge-group 
 // como las especies atomicas 
 for (long int i=0;i<atoms.Size();++i)
 {
  for (int q=0;q<innercontrol.massgroups.Parameters().Size();++q)
     if (atoms[i].Symbol() == innercontrol.massgroups.Parameters()[q]) 
        atoms[i].Mass() = innercontrol.massgroups[innercontrol.massgroups.Parameters()[q]];
  for (int q=0;q<innercontrol.chargegroups.Parameters().Size();++q)
     if (atoms[i].Symbol() == innercontrol.chargegroups.Parameters()[q]) 
        atoms[i].Charge() = innercontrol.chargegroups[innercontrol.chargegroups.Parameters()[q]];
 }
 if (innercontrol.Defined("showinitialconfig"))
 {
  for (long int i=0;i<atoms.Size();++i)
  {
   std::cout << i << ": " << atoms[i].Symbol() << " Mass = " << atoms[i].Mass() << " Charge = " << atoms[i].Charge() << " ";
   std::cout << "Position = " << atoms[i].Position() << '\n';
  }
 }
}

void Application::ApplyPrepares()
{
 if (name == "LPMD") PrintBanner("PREPARE MODULES");
 Array<std::string> prepares = StringSplit(innercontrol["prepare-modules"]);
 for (int p=0;p<prepares.Size();++p)
 {
  std::string id = "prepare"+ToString(p+1);
  SystemModifier & sm = CastModule<SystemModifier>(pluginmanager[id]);
  if (name == "LPMD") pluginmanager[id].Show(std::cout);
  Cell original_cell(simulation->Cell());
  sm.Apply(*simulation);
  if (simulation->Cell() != original_cell) simulation->RescalePositions(original_cell);
 } 
 if (innercontrol.Defined("cellmanager-module")) 
    CastModule<CellManager>(pluginmanager[innercontrol["cellmanager-module"]]).UpdateCell(*simulation);
}

void Application::ApplyFilters()
{
 if (name == "LPMD") PrintBanner("FILTER MODULES");
 Array<std::string> filters = StringSplit(innercontrol["filter-modules"]);
 for (int p=0;p<filters.Size();++p)
 {
  std::string id = "filter"+ToString(p+1);
  SystemFilter & sfilt = CastModule<SystemFilter>(pluginmanager[id]);
  if (name == "LPMD") pluginmanager[id].Show(std::cout);
  Cell original_cell(simulation->Cell());
  bool inverse = false;
  if (pluginmanager[id].Defined("inverse") && (bool(pluginmanager[id]["inverse"]))) inverse = true;
  sfilt.inverted = inverse;
  sfilt.Apply(*simulation);
  if (simulation->Cell() != original_cell) simulation->RescalePositions(original_cell);
 }
}

void Application::SetPotentials()
{
 if (simulation == 0) throw InvalidOperation("Setting potentials on an uninitialized Simulation");
 CombinedPotential & potentials = simulation->Potentials();
 Array<Parameter> pkeys = innercontrol.Potentials().Parameters();
 if (name == "LPMD") PrintBanner("INTERATOMIC POTENTIALS");
 for (int p=0;p<pkeys.Size();++p) 
 {
  Plugin & plugin = pluginmanager[innercontrol.Potentials()[pkeys[p]]];
  Potential & pot = CastModule<Potential>(plugin);
  int spc1 = ElemNum(SplitSpeciesPair(pkeys[p])[0]);
  int spc2 = ElemNum(SplitSpeciesPair(pkeys[p])[0]);
  pot.SetValidSpecies(spc1, spc2);
  potentials.Append(pot);
  if (name == "LPMD") plugin.Show(std::cout);
 } 
}

void Application::Iterate() {  }

void Application::ShowApplicableModules(const std::string & kind)
{
 Array<std::string> modules = StringSplit(innercontrol[kind+"-modules"]);
 for (int p=0;p<modules.Size();++p)
 {
  Module & rawmodule = pluginmanager[modules[p]];
  rawmodule.Show(std::cout);
 }
}

template <typename T> void ApplySteppers(PluginManager & pluginmanager, UtilityControl & control, Simulation & simulation, const std::string & kind)
{
 long currentstep = simulation.CurrentStep();
 Array<std::string> modules = StringSplit(control[kind+"-modules"]);
 for (int p=0;p<modules.Size();++p)
 {
  Module & rawmodule = pluginmanager[modules[p]];
  T & mod = CastModule<T>(rawmodule);
  if (mod.IsActiveInStep(currentstep)) 
  {
   if ((rawmodule.Defined("filterby")) && (rawmodule["filterby"] != "none")) 
   { 
    GlobalSession.DebugStream() << "-> Applying implicit filter on " << modules[p] << '\n';
    GlobalSession.DebugStream() << "-> Filtered plugin details:\n";
    rawmodule.Show(GlobalSession.DebugStream());
    Module & filtering_plugin = pluginmanager[rawmodule["filterby"]];
    GlobalSession.DebugStream() << "-> Filtering plugin details:\n";
    filtering_plugin.Show(GlobalSession.DebugStream());
    Selector<BasicParticleSet> & selector = (CastModule<SystemFilter>(filtering_plugin)).CreateSelector();
    bool inverse = false;
    if (filtering_plugin.Defined("inverse") && (bool(filtering_plugin["inverse"]))) inverse = true;
    simulation.ApplyAtomMask(selector, inverse); 
    Cell original_cell(simulation.Cell());
    if ((mod.end == -1) && control.Defined("steps-number")) mod.end = int(control["steps-number"]);
    mod.Apply(simulation);
    if (simulation.Cell() != original_cell) simulation.RescalePositions(original_cell);
    simulation.RemoveAtomMask();
   }
   else
   {
    Cell original_cell(simulation.Cell());
    if ((mod.end == -1) && control.Defined("steps-number")) mod.end = int(control["steps-number"]);
    mod.Apply(simulation);
    if (simulation.Cell() != original_cell) simulation.RescalePositions(original_cell);
   }
  }
 }
}

void Application::RunModifiers() { ApplySteppers<SystemModifier>(pluginmanager, innercontrol, *simulation, "apply"); }

void Application::RunVisualizers() { ApplySteppers<Visualizer>(pluginmanager, innercontrol, *simulation, "visualize"); }

void Application::ComputeProperties()
{
 long currentstep = simulation->CurrentStep();
 Array<std::string> properties = StringSplit(innercontrol["property-modules"]); 
 for (int p=0;p<properties.Size();++p)
 {
  Plugin & rawmodule = pluginmanager[properties[p]];
  //
  if (pluginmanager.HasType<lpmd::TemporalProperty>(properties[p])) continue;
  //
  lpmd::InstantProperty & prop = CastModule<lpmd::InstantProperty>(rawmodule);
  if (prop.IsActiveInStep(currentstep)) 
  {
   if ((rawmodule.Defined("filterby")) && (rawmodule["filterby"] != "none")) 
   { 
    GlobalSession.DebugStream() << "-> Applying implicit filter on " << properties[p] << '\n';
    GlobalSession.DebugStream() << "-> Filtered plugin details:\n";
    rawmodule.Show(GlobalSession.DebugStream());
    Module & filtering_plugin = pluginmanager[rawmodule["filterby"]];
    GlobalSession.DebugStream() << "-> Filtering plugin details:\n";
    filtering_plugin.Show(GlobalSession.DebugStream());
    Selector<BasicParticleSet> & selector = (CastModule<SystemFilter>(filtering_plugin)).CreateSelector();
    bool inverse = false;
    if (filtering_plugin.Defined("inverse") && (bool(filtering_plugin["inverse"]))) inverse = true;
    simulation->ApplyAtomMask(selector, inverse);
    prop.Evaluate(*simulation, simulation->Potentials());
    simulation->RemoveAtomMask();
   }
   else prop.Evaluate(*simulation, simulation->Potentials());
   lpmd::AbstractValue & value = CastModule<lpmd::AbstractValue>(rawmodule);
   if (bool(pluginmanager[properties[p]]["average"])) value.AddToAverage();
   else value.OutputTo(*(propertystream[p]));
  }
 }
}

void Application::OpenPropertyStreams()
{
 if (name == "LPMD") PrintBanner("INSTANTANEOUS PROPERTIES");
 propertystream.Clear();
 Array<std::string> properties = StringSplit(innercontrol["property-modules"]); 
 for (int p=0;p<properties.Size();++p)
 {
  const Parameter & pluginname = properties[p];
  const std::string filename = pluginmanager[pluginname]["output"];
  propertystream.Append(new std::ofstream(filename.c_str()));
  Module & rawmodule = pluginmanager[properties[p]];
  if (name == "LPMD") rawmodule.Show(std::cout);
  lpmd::AbstractValue & value = CastModule<lpmd::AbstractValue>(rawmodule);
  value.ClearAverage();
 }
}

void Application::ClosePropertyStreams()
{
 Array<std::string> properties = StringSplit(innercontrol["property-modules"]); 
 for (int p=0;p<properties.Size();++p)
 {
  Module & rawmodule = pluginmanager[properties[p]];
  lpmd::AbstractValue & value = CastModule<lpmd::AbstractValue>(rawmodule);
  if (bool(pluginmanager[properties[p]]["average"])) value.OutputAverageTo(*(propertystream[p]));
  delete propertystream[p];
 }
}

void Application::OpenOutputStreams()
{
 Array<std::string> outstreams = StringSplit(innercontrol["output-modules"]); 
 outputstream = new std::ofstream*[outstreams.Size()];
 if (name == "LPMD") PrintBanner("OUTPUT MODULES");
 for (int p=0;p<outstreams.Size();++p)
 {
  const std::string & plugin_id = "output"+ToString(p+1);
  Module & modl = pluginmanager[plugin_id];
  if (!modl.Defined("start")) modl["start"] = "0";
  if (!modl.Defined("end")) modl["end"] = "-1";
  if (name == "LPMD") modl.Show(std::cout);
  const std::string filename = pluginmanager[plugin_id]["file"];
  outputstream[p] = new std::ofstream(filename.c_str());
  CellWriter & cellwriter = CastModule<CellWriter>(pluginmanager[plugin_id]);
  cellwriter.WriteHeader(*(outputstream[p]));
 }
}

void Application::CloseOutputStreams()
{
 Array<std::string> outstreams = StringSplit(innercontrol["output-modules"]); 
 for (int p=0;p<outstreams.Size();++p) delete outputstream[p];
 delete [] outputstream;
}

void Application::SaveCurrentConfiguration()
{
 Array<std::string> outstreams = StringSplit(innercontrol["output-modules"]); 
 for (int p=0;p<outstreams.Size();++p)
 {
  const std::string & plugin_id = "output"+ToString(p+1);
  Module & modl = pluginmanager[plugin_id];
  CellWriter & cellwriter = CastModule<CellWriter>(modl);
  int start = modl["start"], end = modl["end"], each = modl["each"];
  if (Stepper(start,end,each).IsActiveInStep(simulation->CurrentStep()))
     cellwriter.WriteCell(*(outputstream[p]), *simulation);
 }
}

