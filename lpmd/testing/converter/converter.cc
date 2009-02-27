//
//
//

#include "converter.h"
#include "cmdline.h"
#include "config.h"

#include <lpmd/session.h>
#include <lpmd/util.h>
#include <lpmd/containable.h>
#include <lpmd/instantproperty.h>
#include <lpmd/scalartable.h>
#include <lpmd/cellformat.h>
#include <lpmd/cellmanager.h>
#include <lpmd/potentialarray.h>
#include <lpmd/simulationcell.h>
#include <lpmd/systemmodifier.h>

#include <iostream>
#include <sstream>

using namespace lpmd;

Converter::Converter(): CommonHandler("lpmd-converter", "LPMD Converter")
{
 CommonInputReader * inp = new ConverterInputReader(pluginman, GlobalSession);
 SetInputReader(*inp);
}

Converter::~Converter() 
{
 CommonInputReader & param = GetInputReader();
 CommonInputReader * inp = &param;
 delete inp;
}

void Converter::LoadModules()
{
 CommonHandler::LoadModules();
 CommonInputReader & param = GetInputReader();
 //
 // Carga modulos SystemModifier
 // 
 param["apply-list"] = param["special-list"] + param["apply-list"];
 std::vector<std::string> modifiers = SplitTextLine(param["apply-list"]);
 for (unsigned int i=0;i<modifiers.size();++i)
 {
  Module & pmod = pluginman[modifiers[i]];
  SystemModifier & smp = CastModule<SystemModifier>(pmod);
  smlist.push_back(&smp);
  pmod.SetUsed();
 }
}

//
//
//
void Converter::Initialize()
{
 CommonHandler::Initialize();
 CommonInputReader & param = GetInputReader();
 try
 {
  // Primero prueba si el modulo input es del tipo CellReader
  // En este caso se pueden leer muchas configuraciones
  CellReader & cread = CastModule<CellReader>(pluginman[param["input-module"]]);
  if (GlobalSession["debug"] != "none") GlobalSession.DebugStream() << "-> Loading input file: " << param["input-file"] << '\n';
  configs.push_back(SimulationCell(*scell));
  if (param.GetBool("replacecell")) pluginman[param["input-module"]].AssignParameter("replacecell", "true");
  cread.ReadMany(param["input-file"], configs); 
 }
 catch (InvalidModuleType & e)
 {
  // Ahora prueba si es del tipo CellGenerator, se generara solo una configuracion
  CellGenerator & cgen = CastModule<CellGenerator>(pluginman[param["input-module"]]);
  if (GlobalSession["debug"] != "none") GlobalSession.DebugStream() << "-> Creating input configuration..." << '\n';
  configs.push_back(SimulationCell(*scell));
  if (param.GetBool("replacecell")) pluginman[param["input-module"]].AssignParameter("replacecell", "true");
  cgen.Generate(configs[0]);
 }
 if (GlobalSession["debug"] != "none") 
 {
  std::ostream & debug = GlobalSession.DebugStream();
  debug << "-> Read " << configs.size() << " configurations." << '\n';
  debug << "-> Configuration 0 has " << configs[0].size() << " atoms\n";
 }
 pluginman.UnloadPlugin(param["input-module"]);
}

//
//
//
void Converter::Process()
{
 PotentialArray & p_array = GetPotentialArray();
 CommonInputReader & param = GetInputReader();
 CellManager * cm = NULL;
 if (param.Defined("cellmanager-module")) 
 { 
  // Carga el plugin CellManager y lo asigna a la celda de simulacion
  Module & pmod = pluginman[param["cellmanager-module"]];
  cm = & (CastModule<CellManager>(pmod));
  pmod.SetUsed();
 }

 // Carga los modulos de "prepare"
 SimulationCell & sc = GetCell();
 for (std::list<ModuleInfo>::const_iterator it=param.preparelist.begin();it != param.preparelist.end();++it)
 {
  const ModuleInfo & minf = *it;
  pluginman.LoadPlugin(minf.name, minf.id, minf.args);
  pluginman[minf.id].SetUsed();  
 }
 
 for (unsigned long i=0;i<configs.size();++i)
 {
  if (param.GetString("replacecell") == "false") configs[i].SetCell(*scell);
  if (cm != NULL) configs[i].SetCellManager(*cm);
  configs[i].UseDistanceCache(GlobalSession.GetBool("distancecache"));
  // Aplica los modulos de "prepare"
  for (std::list<ModuleInfo>::const_iterator it=param.preparelist.begin();it != param.preparelist.end();++it)
  {
   const ModuleInfo & minf = *it;
   SystemModifier & sm = CastModule<SystemModifier>(pluginman[minf.id]);
   sm.Apply(configs[i]);
  }
 }

 // Descarga los modulos de "prepare"
 for (std::list<ModuleInfo>::const_iterator it=param.preparelist.begin();it != param.preparelist.end();++it)
 {
  const ModuleInfo & minf = *it;
  pluginman.UnloadPlugin(minf.id);
 }

 if (GlobalSession["debug"] != "none")
 {
  std::ostream & debug = GlobalSession.DebugStream();
  if (smlist.size() > 0) debug << "-> Will apply the following modifiers: \n";
  for (std::vector<SystemModifier *>::iterator it=smlist.begin();it!=smlist.end();++it)
  {
   SystemModifier & smp = *(*it);
   Module & mod = dynamic_cast<Module &>(smp); // no es necesario CastModule aqui
   mod.Show(debug);
   debug << '\n';
  }
 }

 //
 // Applica los SystemModifiers
 //
 for (unsigned long i=0;i<configs.size();++i)
 {
  if (param.GetString("replacecell") == "false") configs[i].SetCell(*scell);
  if (cm != NULL) configs[i].SetCellManager(*cm);
  configs[i].UseDistanceCache(GlobalSession.GetBool("distancecache"));
  for (std::vector<SystemModifier *>::iterator it=smlist.begin();it!=smlist.end();++it)
  {
   SystemModifier & smp = *(*it);
   Module & mod = dynamic_cast<Module &>(smp); // no es necesario CastModule aqui
   if ((smp.interval == -1) || (MustDo(i, smp.start_step, smp.end_step, smp.interval))) 
      smp.Apply(configs[i]);
  }
 }
}

void Converter::Finish()
{
 //
 // This loads the output (cell writer) modules
 //
 CommonInputReader & param = GetInputReader();
 if (param.outputlist.size() == 0) EndWithError("No CellWriter plugin was assigned. Maybe you forgot an \"output\" statement?");
 // Carga los modulos para escritura 
 for (std::list<ModuleInfo>::const_iterator it=param.outputlist.begin();it != param.outputlist.end();++it)
 {
  const ModuleInfo & minf = *it;
  pluginman.LoadPlugin(minf.name, minf.id, minf.args);
  pluginman[minf.id].SetUsed();
  CellWriter & cwrite = CastModule<CellWriter>(pluginman[minf.id]);
  if (GlobalSession["debug"] != "none") GlobalSession.DebugStream() << "-> Writing output file: " << cwrite.GetFile() << '\n';
  cwrite.WriteMany(cwrite.GetFile(), configs); 
  pluginman.UnloadPlugin(minf.id);
 }
}

//
// Main Program
//
int main(int argc, char **argv)
{
 Converter handler;
 ConverterCmdLineParser clp(argc, argv);
 try { handler.Execute(clp); }
 catch (std::exception & ex) { EndWithError(ex.what()); }
 return 0;
}

