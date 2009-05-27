//
//
//

#include "input.h"

#include <lpmd/util.h>
#include <iostream>

using namespace lpmd;

void LPMDInputReader::Read(std::string inpfile, const ParamList & optvars)
{
 inside_typeblock = false;
 CommonInputReader::Read(inpfile, optvars);
 // si no se definio 'monitor', se pone uno por defecto aqui
 if (monapply.size() == 0)
 {  
  monapply.push_back(MonitorApplyInfo("kinetic-energy,potential-energy,total-energy", 0, GetInteger("steps-n"), 10, ""));
 }
}

//
//
//
LPMDInputReader::LPMDInputReader(PluginManager & pm, Map & params): CommonInputReader(pm, params) 
{ 
 pluginman = &pm;

 DeclareStatement("type", "name");
 DeclareStatement("atom", "type from to start");
 DeclareStatement("restore", "dumpfile");
 DeclareStatement("steps", "n");
 DeclareStatement("dumping", "each dumpfile");
 DeclareStatement("monitor", "properties start end each output");
 DeclareStatement("average", "properties start end each over output");
 DeclareStatement("integrator", "module start");
 DeclareStatement("apply", "module start end each");
 DeclareStatement("visualize", "module start end each");
 DeclareStatement("property", "module start end each");
 DeclareStatement("charge", "symbol charge");

 // Some default values
 Map & param = (*this);

 param["monitor-start"] = "0";
 param["monitor-end"] = "-1";
 param["monitor-each"] = "10";
 param["average-start"] = "0";
 param["average-end"] = "-1";
 param["average-each"] = "10";
 param["average-over"] = "100";
 param["showcoords"] = "true";
 param["showunused"] = "true";
 param["dumping-each"] = "10000";
 param["dumping-dumpfile"] = "restore.dump";
 param["restore-dumpfile"] = "";
 param["type-list"] = "";
 param["visualize-list"] = "";
 param["integrator-list"] = "";
}


//
//
//
int LPMDInputReader::OnStatement(const std::string & name, const std::string & keywords, bool regular)
{
 Map & param = (*this);
 PluginManager & pm = *(pluginman);
 if (regular)
 {
  if (name == "monitor")
  {
   MonitorApplyInfo mon(param["monitor-properties"], param.GetInteger("monitor-start"), param.GetInteger("monitor-end"), param.GetInteger("monitor-each"), param["monitor-output"]);
   monapply.push_back(mon);
   param.Remove("monitor-properties");
   param["monitor-start"] = "0";
   param["monitor-end"] = "-1";
   param["monitor-each"] = "10";
   param.Remove("monitor-output");
  }
  else if (name == "average")
  {
   RunningAverageApplyInfo av(param["average-properties"], param.GetInteger("average-start"), param.GetInteger("average-end"), param.GetInteger("average-each"), param["average-output"], param.GetInteger("average-over"));
   ravapply.push_back(av);
   param.Remove("average-properties");
   param["average-start"] = "0";
   param["average-end"] = "-1";
   param["average-each"] = "10";
   param["average-over"] = "100";
   param.Remove("average-output");
  }
  else if (name == "atom")
  {
   if (! param.Defined("atom-start")) param["atom-start"] = "1";
   AtomTypeApplyInfo atinf;
   atinf.t = NULL;
   atinf.from_index = param.GetInteger("atom-from");
   atinf.to_index = param.GetInteger("atom-to");
   atinf.start = param.GetInteger("atom-start");
   atinf.name = param["atom-type"];
   atapply.push_back(atinf);
  }
  else if (name == "charge") param["charge-"+param["charge-symbol"]] = param["charge-charge"];
  else if ((name == "visualize") || (name == "integrator"))
  {
   std::string tname = param[name+"-module"];
   param[tname+"-args"] = keywords;
   param[name+"-list"] = param[name+"-list"] + tname + " ";
  }
  else return CommonInputReader::OnStatement(name, keywords, regular);
 }
 else
 {
  // Instrucciones irregulares y no validas 
  if (name == "typeblock")
  {
   param["type-name"] = GetNextWord();
   param["type-list"] = param["type-list"] + param["type-name"] + " ";
   param["atom-"+param["type-name"]] = "";
   param["type-args"] = "";
   while (words.size() > 0) param["type-args"] = param["type-args"] + (GetNextWord()+" ");
   param["type-"+param["type-name"]+"-args"] = param["type-args"];
   param.Remove("type-name");
   param.Remove("type-args");
  }
  else return CommonInputReader::OnStatement(name, keywords, regular);
 }
 return 0; // status OK
}

