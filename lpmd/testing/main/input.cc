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
}

//
//
//
LPMDInputReader::LPMDInputReader(PluginManager & pm): CommonInputReader(pm) 
{ 
 pluginman = &pm;

 DeclareStatement("type", "name");
 DeclareStatement("atom", "type from to start");
 DeclareStatement("restore", "dumpfile");
 DeclareStatement("steps", "n");
 DeclareStatement("dumping", "each dumpfile");
 DeclareStatement("monitor", "properties start end each output");
 DeclareStatement("integrator", "module start");
 DeclareStatement("apply", "module start end each");
 DeclareStatement("visualize", "module start end each");
 DeclareStatement("property", "module start end each");
 DeclareStatement("charge", "symbol charge");

 // Some default values
 ParamList & param = (*this);

 param["showcoords"] = "true";
 param["showunused"] = "true";
 param["monitor-properties"] = "kinetic-energy,potential-energy,total-energy";
 param["monitor-each"] = "10";
 param["monitor-output"] = "";
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
 ParamList & param = (*this);
 PluginManager & pm = *(pluginman);
 if (regular)
 {
  if (name == "type")
  {
   inside_typeblock = true;
   param["atom-"+param["type-name"]] = "";
   param["type-args"] = "";
  }
  else if (name == "atom")
  {
   if (! param.Defined("atom-start")) param["atom-start"] = "1";
   AtomTypeApplyInfo atinf;
   atinf.t = NULL;
   atinf.from_index = param.GetInteger("atom-from");
   atinf.to_index = param.GetInteger("atom-to");
   atinf.start_step = param.GetInteger("atom-start");
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
  if (inside_typeblock == true)
  {
   if (name == "endtype")
   {
    // Procesa el caso cuando se esta dentro de un bloque type / endtype
    param["type-list"] = param["type-list"] + param["type-name"] + " ";
    param["type-"+param["type-name"]+"-args"] = param["type-args"];
    param.Remove("type-name");
    param.Remove("type-args");
    inside_typeblock = false;
   }
   else if (name != param["type-name"])
   {
    param["type-args"] = param["type-args"] + name + " ";
    while (words.size() > 0) param["type-args"] = param["type-args"] + GetNextWord() + " ";
   }
  }
  else return CommonInputReader::OnStatement(name, keywords, regular);
 }
 return 0; // status OK
}

