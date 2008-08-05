//
//
//

#include "visualhandler.h"
#include "cmdline.h"
#include "config.h"

#include <lpmd/util.h>
#include <lpmd/cellformat.h>
#include <lpmd/cellmanager.h>
#include <lpmd/simulationcell.h>
#include <lpmd/systemmodifier.h>
#include <lpmd/visualizer.h>

#include <iostream>
#include <sstream>

using namespace lpmd;

VisualHandler::VisualHandler(): CommonHandler("lpmd-visualizer", "LPMD Visualizer")
{
 CommonInputReader * inp = new VisualizerInputReader(pluginman);
 SetInputReader(*inp);
}

VisualHandler::~VisualHandler() 
{
 CommonInputReader & param = GetInputReader();
 CommonInputReader * inp = &param;
 delete inp;
}

void VisualHandler::LoadModules()
{
 CommonHandler::LoadModules();
 CommonInputReader & param = GetInputReader();
 //
 // Carga modulos Visualizer
 // 
 param["visualize-list"] = param["special-list"] + param["visualize-list"];
 std::vector<std::string> visualizers = SplitTextLine(param["visualize-list"]);
 for (unsigned int i=0;i<visualizers.size();++i)
 {
  Module & pmod = pluginman[visualizers[i]];
  Visualizer & vis = CastModule<Visualizer>(pmod);
  vis.interval = -1;
  vislist.push_back(&vis);
  pmod.SetUsed();
 }
}

void VisualHandler::Initialize()
{
 CommonHandler::Initialize();
}

//
//
//
void VisualHandler::Process()
{
 PotentialArray & p_array = GetPotentialArray();
 CommonInputReader & param = GetInputReader();
 CellManager * cm = NULL;

 // Carga los modulos de "prepare"
 SimulationCell & sc = GetCell();
 for (std::list<ModuleInfo>::const_iterator it=param.preparelist.begin();it != param.preparelist.end();++it)
 {
  const ModuleInfo & minf = *it;
  pluginman.LoadPlugin(minf.name, minf.id, minf.args);
  pluginman[minf.id].SetUsed();  
 }

 if (param.Defined("cellmanager-module")) 
 { 
  // Carga el plugin CellManager y lo asigna a la celda de simulacion
  Module & pmod = pluginman[param["cellmanager-module"]];
  cm = & (CastModule<CellManager>(pmod));
  pmod.SetUsed();
 }

 if (Verbose())
 {
  if (vislist.size() > 0) std::cerr << "-> Will use the following visualizers: \n";
  for (std::vector<Visualizer *>::iterator it=vislist.begin();it!=vislist.end();++it)
  {
   Visualizer & vis = *(*it);
   Module & mod = dynamic_cast<Module &>(vis); // no es necesario CastModule aqui
   mod.Show(std::cerr);
   std::cerr << '\n';
  }
 }

 step = 0;
 try
 {
  // Primero prueba si el modulo input es del tipo CellReader
  // En este caso se pueden leer muchas configuraciones
  CellReader & cread = CastModule<CellReader>(pluginman[param["input-module"]]);
  if (Verbose()) std::cerr << "-> Loading input file: " << param["input-file"] << '\n';
  std::ifstream is(param["input-file"].c_str());
  if (! is.good()) throw FileNotFound(param["input-file"]);
  if (param.GetBool("replacecell")) pluginman[param["input-module"]].AssignParameter("replacecell", "true");
  cread.ReadHeader(is);
  while (cread.ReadCell(is, *scell)) 
  {
   scell->NumEspec();
   scell->AssignIndex();
   ProcessConfig();
   scell->Clear();
  }
 }
 catch (InvalidModuleType & e)
 {
  // Ahora prueba si es del tipo CellGenerator, se generara solo una configuracion
  CellGenerator & cgen = CastModule<CellGenerator>(pluginman[param["input-module"]]);
  if (Verbose()) std::cerr << "-> Creating input configuration..." << '\n';
  if (param.GetBool("replacecell")) pluginman[param["input-module"]].AssignParameter("replacecell", "true");
  cgen.Generate(*scell);
  scell->NumEspec();
  scell->AssignIndex();
  ProcessConfig();
  scell->Clear();
 }
}

void VisualHandler::ProcessConfig()
{ 
 CellManager * cm = &(scell->GetCellManager());
 CommonInputReader & param = GetInputReader();
 if (cm != NULL) scell->SetCellManager(*cm);
 scell->UseDistanceCache(param.GetBool("distancecache"));
 // Aplica los modulos de "prepare"
 for (std::list<ModuleInfo>::const_iterator it=param.preparelist.begin();it != param.preparelist.end();++it)
 {
  const ModuleInfo & minf = *it;
  SystemModifier & sm = CastModule<SystemModifier>(pluginman[minf.id]);
  sm.Apply(*scell);
 }
 //
 // Aplica los Visualizers
 //
 for (std::vector<Visualizer *>::iterator it=vislist.begin();it!=vislist.end();++it)
 {
  Visualizer & vis = *(*it);
  Module & mod = dynamic_cast<Module &>(vis); // no es necesario CastModule aqui
  if ((vis.interval == -1) || (MustDo(step, vis.start_step, vis.end_step, vis.interval))) vis.Apply(*this);
 }
 step++;
}

void VisualHandler::Finish()
{
 //
 CommonInputReader & param = GetInputReader();
 pluginman.UnloadPlugin(param["input-module"]);
 // Descarga los modulos de "prepare"
 for (std::list<ModuleInfo>::const_iterator it=param.preparelist.begin();it != param.preparelist.end();++it)
 {
  const ModuleInfo & minf = *it;
  pluginman.UnloadPlugin(minf.id);
 }
}

//
// Main Program
//
int main(int argc, char **argv)
{
 VisualHandler handler;
 VisualizerCmdLineParser clp(argc, argv);
 try { handler.Execute(clp); }
 catch (std::exception & ex) { EndWithError(ex.what()); }
 return 0;
}

