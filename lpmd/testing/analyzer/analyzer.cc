//
//
//

#include "analyzer.h"
#include "cmdline.h"
#include "config.h"

#include <lpmd/util.h>
#include <lpmd/session.h>
#include <lpmd/stepper.h>
#include <lpmd/containable.h>
#include <lpmd/value.h>
#include <lpmd/matrix.h>
#include <lpmd/instantproperty.h>
#include <lpmd/cellreader.h>
#include <lpmd/cellmanager.h>
#include <lpmd/systemmodifier.h>
#include <lpmd/potentialarray.h>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace lpmd;

Analyzer::Analyzer(): CommonHandler("lpmd-analyzer", "LPMD Analyzer")
{
 CommonInputReader * inp = new AnalyzerInputReader(pluginman, GlobalSession);
 SetInputReader(*inp);
}

Analyzer::~Analyzer() 
{
 CommonInputReader & param = GetInputReader();
 std::vector<std::string> properties = StringSplit< std::vector<std::string> >(param["property-list"]);
 for (unsigned int i=0;i<properties.size();++i) delete propfiles[properties[i]];
 CommonInputReader * inp = &param;
 delete inp;
}

void Analyzer::LoadModules()
{
 CommonHandler::LoadModules();
 CommonInputReader & param = GetInputReader();
 param["property-list"] = param["special-list"] + param["property-list"];
 std::vector<std::string> properties = StringSplit< std::vector<std::string> >(param["property-list"]);
 for (unsigned int i=0;i<properties.size();++i)
 {
  Module & pmod = pluginman[properties[i]];
  try
  {
   InstantProperty & ip = CastModule<InstantProperty>(pmod);
   iproplist.push_back(&ip);
  }
  catch (const std::exception & ex) 
  {
   try
   {
    TemporalProperty & tp = CastModule<TemporalProperty>(pmod);
    tproplist.push_back(&tp);
   }
   catch(const std::exception & ex) { EndWithError("Module "+properties[i]+" is not a property"); }
  }
  propfiles[properties[i]] = new std::ofstream(pmod["output"].c_str(), std::ios::trunc);
  pmod.SetUsed();
 }
}

//
//
//
void Analyzer::Initialize()
{
 CommonHandler::Initialize();
 CommonInputReader & param = GetInputReader();
 try
 {
  // Primero prueba si el modulo input es del tipo CellReader
  // En este caso se pueden leer muchas configuraciones
  CellReader & cread = CastModule<CellReader>(pluginman[param["input-module"]]);
  if (Verbose()) std::cerr << "-> Loading input file: " << param["input-file"] << '\n';
  configs.push_back(SimulationCell(*scell));
  if (param.GetBool("replacecell")) pluginman[param["input-module"]].AssignParameter("replacecell", "true");
  cread.ReadMany(param["input-file"], configs); 
 }
 catch (InvalidModuleType & e)
 {
  // Ahora prueba si es del tipo CellGenerator, se generara solo una configuracion
  CellGenerator & cgen = CastModule<CellGenerator>(pluginman[param["input-module"]]);
  if (Verbose()) std::cerr << "-> Creating input configuration..." << '\n';
  configs.push_back(SimulationCell(*scell));
  if (param.GetBool("replacecell")) pluginman[param["input-module"]].AssignParameter("replacecell", "true");
  cgen.Generate(configs[0]);
  configs[0].NumEspec();
  configs[0].AssignIndex();
 }
 if (Verbose()) 
  std::cerr << "-> Read " << configs.size() << " configuration(s)." << '\n';
 pluginman.UnloadPlugin(param["input-module"]);
}

//
//
//
void Analyzer::Process()
{
 PotentialArray & p_array = GetPotentialArray();
 CommonInputReader & param = GetInputReader();
 CellManager * cm = NULL;
 if (param.Defined("cellmanager-module")) 
 {
  // Carga el plugin CellManager y lo asigna a la celda de simulacion
  Module & pmod = pluginman[param["cellmanager-module"]];
  cm = &(CastModule<CellManager>(pmod));
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
  configs[i].UseDistanceCache(param.GetBool("distancecache"));
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

 if (Verbose())
 { 
  ShowConfigsInfo(configs);
  if (iproplist.size() > 0) std::cerr << "-> Will calculate the following instant properties: \n";
  for (std::vector<InstantProperty *>::iterator it=iproplist.begin();it!=iproplist.end();++it)
  {
   std::ostream * outp;
   InstantProperty & prop = *(*it);
   Module & propmod = dynamic_cast<Module &>(prop); // no es necesario CastModule aqui
   propmod.Show(std::cerr);
   std::cerr << '\n';
  }
  if (tproplist.size() > 0) std::cerr << "-> Will calculate the following temporal properties: \n";
  for (std::vector<TemporalProperty *>::iterator it=tproplist.begin();it!=tproplist.end();++it)
  {
   std::ostream * outp;
   TemporalProperty & prop = *(*it);
   Module & propmod = dynamic_cast<Module &>(prop); // no es necesario CastModule aqui
   propmod.Show(std::cerr);
   std::cerr << '\n';
  }
 }
 //
 // Calcula las propiedades instantaneas
 //
 for (std::vector<InstantProperty *>::iterator it=iproplist.begin();it!=iproplist.end();++it)
 {
  std::ostream * outp;
  InstantProperty & prop = *(*it);
  Module & propmod = dynamic_cast<Module &>(prop); // no es necesario CastModule aqui
  if (propmod.Defined("output") && (propmod["output"] != ""))  
  {
   outp = new std::ofstream(propmod["output"].c_str());
   if (Verbose()) std::cerr << "-> Writing output of " << propmod.Name() << " to " << propmod["output"] << '\n';
  }
  else outp = &(std::cout);
  Value<Matrix> & propval = CastModule< Value<Matrix> >(propmod);
  IContainable & icont = CastModule<IContainable>(propmod);
  for (unsigned long i=0;i<configs.size();++i)
  {
   if ((prop.each == -1) || (prop.IsActiveInStep(i)))
   {
    prop.Evaluate(configs[i], p_array);
    if (propmod.Defined("average") && (propmod["average"] == "true")) propval.AddToAverage();
    else icont.OutputTo(*outp);
   }
  }
  if (propmod.Defined("average") && (propmod["average"] == "true")) (*outp) << propval;
  if (propmod.Defined("output") && (propmod["output"] != "")) delete outp;
 }
 //
 // Calcula las propiedades temporales
 //
 for (std::vector<TemporalProperty *>::iterator it=tproplist.begin();it!=tproplist.end();++it)
 {
  std::ostream * outp;
  TemporalProperty & prop = *(*it);
  Module & propmod = dynamic_cast<Module &>(prop); // no es necesario CastModule aqui
  if (propmod.Defined("output") && (propmod["output"] != ""))
  {
   outp = new std::ofstream(propmod["output"].c_str());
   if (Verbose()) std::cerr << "-> Writing output of " << propmod.Name() << " to " << propmod["output"] << '\n';
  }
  else outp = &(std::cout);
  Value<Matrix> & propval = CastModule< Value<Matrix> >(propmod);
  IContainable & icont = CastModule<IContainable>(propmod);
  prop.Evaluate(configs, p_array);
  const Matrix & v = propval.CurrentValue();
  (*outp) << v;
  if (propmod.Defined("output") && (propmod["output"] != "")) delete outp;
 }
}

void Analyzer::Finish() { }

//
// Main Program
//
int main(int argc, char **argv)
{
 Analyzer handler;
 AnalyzerCmdLineParser clp(argc, argv);
 try { handler.Execute(clp); }
 catch (const std::exception & ex) { EndWithError(ex.what()); }
 return 0;
}

