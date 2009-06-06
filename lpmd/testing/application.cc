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
#include <lpmd/session.h>

#include <iostream>
#include <fstream>
#include <cstdlib>

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
 //
 OpenOutputStreams();
 Iterate();
 CloseOutputStreams();
 //
 return 0;
}

void Application::ProcessControl(int argc, const char * argv[])
{
 QuickModeParser quick;
 quick.Parse(argc, argv);
 if (quick.Defined("help")) ShowHelp();
 else
 {
  ParamList options;
  if (quick.Arguments().Size() == 1)
  {
   std::istringstream generatedcontrol(quick.FormattedAsControlFile());
   innercontrol.Read(generatedcontrol, options, "quickmode"); 
   if (pluginmanager.IsLoaded("help_plugin")) ShowPluginHelp();
   else if (!innercontrol.Defined("cell-type")) ShowHelp();
  }
  else innercontrol.Read(quick.Arguments()[1], options);
 }
 if (quick.Defined("verbose")) 
 {
  GlobalSession.AssignParameter("debug", "stderr");
  innercontrol["verbose"] = "true";
 }
 GlobalSession.DebugStream() << pluginmanager << '\n';
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
  cell[0] = e1;
  cell[1] = e2;
  cell[2] = e3;
 }
 else if (celltype == "cubic")
 {
  double length = double(innercontrol["cell-a"]);
  double scale = double(innercontrol["cell-scale"]);
  cell[0] = length*scale*e1;
  cell[1] = length*scale*e2;
  cell[2] = length*scale*e3;
 }
 else if (celltype == "crystal")
 {
  double box[3];
  box[0] = double(innercontrol["cell-a"]);
  box[1] = double(innercontrol["cell-b"]);
  box[2] = double(innercontrol["cell-c"]);
  double scale = double(innercontrol["cell-scale"]);
  for (int q=0;q<3;++q) cell[q] = box[q]*scale*identity[q];
 }
 else 
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
}

void Application::ConstructSimulation()
{
 simulation = &(SimulationBuilder::CreateGeneric());
 #warning "SimulationBuilder::CreateGeneric, buscar metodo para crear una Simulation especializada"
 for (int q=0;q<3;++q) simulation->Cell()[q] = cell[q];
}

void Application::FillAtoms()
{
 CellGenerator & cg = CastModule<CellGenerator>(pluginmanager["input1"]);
 cg.Generate(*simulation);
 if (innercontrol.Defined("cellmanager-module"))
 {
  simulation->SetCellManager(CastModule<CellManager>(pluginmanager[innercontrol["cellmanager-module"]]));
 }
}

void Application::FillAtomsFromCellReader()
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
 }
 catch (Error & e)
 {
  CellGenerator & generator = CastModule<CellGenerator>(inputmodule);
  generator.Generate(*simulation);
 }
 if (innercontrol.Defined("cellmanager-module"))
    simulation->SetCellManager(CastModule<CellManager>(pluginmanager[innercontrol["cellmanager-module"]]));
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
 Array<std::string> prepares = StringSplit(innercontrol["prepare-modules"]);
 for (int p=0;p<prepares.Size();++p)
 {
  std::string id = "prepare"+ToString(p+1);
  SystemModifier & sm = CastModule<SystemModifier>(pluginmanager[id]);
  sm.Apply(*simulation);
 } 
}

void Application::ApplyFilters()
{
 Array<std::string> filters = StringSplit(innercontrol["filter-modules"]);
 for (int p=0;p<filters.Size();++p)
 {
  std::string id = "filter"+ToString(p+1);
  SystemFilter & sfilt = CastModule<SystemFilter>(pluginmanager[id]);
  sfilt.Apply(*simulation);
 }
}

void Application::SetPotentials()
{
 if (simulation == 0) throw InvalidOperation("Setting potentials on an uninitialized Simulation");
 CombinedPotential & potentials = simulation->Potentials();
 Array<Parameter> pkeys = innercontrol.Potentials().Parameters();
 for (int p=0;p<pkeys.Size();++p) 
 {
  Potential & pot = CastModule<Potential>(pluginmanager[innercontrol.Potentials()[pkeys[p]]]);
  int spc1 = ElemNum(SplitSpeciesPair(pkeys[p])[0]);
  int spc2 = ElemNum(SplitSpeciesPair(pkeys[p])[0]);
  pot.SetValidSpecies(spc1, spc2);
  potentials.Append(pot);
 } 
}

void Application::Iterate() {  }

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
    simulation.ApplyAtomMask(selector); 
    mod.Apply(simulation);
    simulation.RemoveAtomMask();
   }
   else mod.Apply(simulation);
  }
 }
}

void Application::RunModifiers() { ApplySteppers<SystemModifier>(pluginmanager, innercontrol, *simulation, "apply"); }

void Application::RunVisualizers() { ApplySteppers<Visualizer>(pluginmanager, innercontrol, *simulation, "visualize"); }

void Application::OpenOutputStreams()
{
 Array<std::string> outstreams = StringSplit(innercontrol["output-modules"]); 
 outputstream = new std::ofstream*[outstreams.Size()];
 for (int p=0;p<outstreams.Size();++p)
 {
  const std::string & plugin_id = "output"+ToString(p+1);
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

