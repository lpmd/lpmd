//
//
//

#include <iostream>

#include <lpmd/util.h>
#include "input.h"

using namespace lpmd;

//
//
//
LPMDInputReader::LPMDInputReader(PluginManager & pm) 
{ 
 pluginman = &pm;
 
 DeclareStatement("use", "module as alias");
 DeclareStatement("type", "name");
 DeclareStatement("atom", "type start end");
 DeclareStatement("plugindir", "dir");
 DeclareStatement("include", "systemfile");
 DeclareStatement("cell crystal", "a b c alpha beta gamma");
 DeclareStatement("cell vector", "ax ay az bx by bz cx cy cz");
 DeclareStatement("restore", "dumpfile");
 DeclareStatement("repeat", "x y z");
 DeclareStatement("periodic", "x y z");
 DeclareStatement("steps", "n");
 DeclareStatement("dumping", "each dumpfile");
 DeclareStatement("monitor", "each");
 DeclareStatement("temperature", "t");
 DeclareStatement("potential", "module a b");
 DeclareStatement("integrator", "module start");
 DeclareStatement("distcache", "activated");
 DeclareStatement("apply", "module start end each");
 DeclareStatement("visualize", "module start end each");
 DeclareStatement("property", "module start end each");
 DeclareStatement("charge", "symbol charge");

 // Some default values
 ParamList & param = (*this);

 param["monitor-each"] = "10";
 param["dumping-each"] = "10000";
 param["dumping-dumpfile"] = "restore.dump";
 param["restore-dumpfile"] = "";
 param["type-list"] = "";
 param["property-list"] = "";
 param["apply-list"] = "";
 param["visualize-list"] = "";
 param["integrator-list"] = "";
}

LPMDInputReader::LPMDInputReader(std::string sysfile) { Read(sysfile); }

LPMDInputReader::~LPMDInputReader() { }

//
//
//
void LPMDInputReader::Read(std::string sysfile)
{
 inside_useblock = false;
 inside_typeblock = false;
 InputFile::Read(sysfile);
 ParamList & param = (*this);
 // Esto agrega los argumentos pasados con "property" y "apply" a la lista de argumentos en ModuleInfo (pasados con "use") 
 for (std::list<ModuleInfo>::iterator it=uselist.begin();it!=uselist.end();++it)
 {
  ModuleInfo & minf = *(it);
  if (param.Defined(minf.id+"-args")) minf.args += (" " + param[minf.id+"-args"]);
  param.Remove(minf.id+"-args");
 }
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
  // Instrucciones regulares
  if (name == "use")
  {
   if (param.Defined("use-alias")) pm.DefineAlias(param["use-alias"], param["use-module"]);
   else param["use-alias"] = param["use-module"];
   inside_useblock = true;
   param["use-args"] = "";
  }
  else if (name == "type")
  {
   inside_typeblock = true;
   param["atom-"+param["type-name"]] = "";
   param["type-args"] = "";
  }
  else if (name == "atom")
  {
   param["atom-"+param["atom-type"]] = param["atom-"+param["atom-type"]]+param["atom-start"]+"-"+param["atom-end"]+" ";
  }
  else if (name == "include") Read(param["include-systemfile"]);
  else if (name == "plugindir") pm.AddToPluginPath(param["plugindir-dir"]);
  else if (name == "potential") 
     param["potential-list"] = param["potential-list"]+param["potential-a"]+"#"+param["potential-b"]+"#"+param["potential-module"]+" ";
  else if (name == "charge") param["charge-"+param["charge-symbol"]] = param["charge-charge"];
  else if ((((name == "property") || (name == "apply")) || (name == "visualize")) || (name == "integrator"))
  {
   std::string tname = param[name+"-module"];
   param[tname+"-args"] = keywords;
   param[name+"-list"] = param[name+"-list"] + tname + " ";
  }
 }
 else
 {
  // Instrucciones irregulares y no validas 
  if (inside_useblock == true)
  {
   if (name == "enduse") 
   { 
    // Procesa el caso cuando se esta dentro de un bloque use / enduse
    ModuleInfo minf(param["use-module"], param["use-alias"], param["use-args"]);
    uselist.push_back(minf);
    param.Remove("use-module");
    param.Remove("use-alias");
    param.Remove("use-args");
    param.Remove("use-as");
    inside_useblock = false;
   }
   else if (name != param["use-module"])
   {
    param["use-args"] = param["use-args"] + name + " ";
    while (words.size() > 0) param["use-args"] = param["use-args"] + GetNextWord() + " ";
   }
  }
  else if (inside_typeblock == true)
  {
   if (name == "endtype")
   {
    // Procesa el caso cuando se esta dentro de un bloque type / endtype
    param["type-list"] = param["type-list"] + param["type-name"] + " ";
    param["type-"+param["type-name"]+"-args"] = param["type-args"];
    param.Remove("type-name");
    param.Remove("type-args");
    inside_useblock = false;
   }
   else if (name != param["type-name"])
   {
    param["type-args"] = param["type-args"] + name + " ";
    while (words.size() > 0) param["type-args"] = param["type-args"] + GetNextWord() + " ";
   }
  }
  else if ((name == "input") || (name == "output"))
  {
   std::list<std::string> tmpwords = words;
   std::string validkws = "module ";
   ParseCommandArguments(name, validkws);
   std::string pluginkws = pm.GetPluginKeywords(param[name+"-module"]);
   words = tmpwords;
   std::string keyw = ParseCommandArguments(name, validkws+pluginkws);
   param[name+"-moduleargs"] = keyw;
  }
  else return 1; // Unexpected statement
 }
 return 0; // status OK
}

//
//
//
int LPMDInputReader::CheckConsistency()
{
 ParamList & param = (*this);
 /*
  * Check propiedades de la Celda :
  */
 if(param.Defined("cell-a"))
 {
  if(GetDouble("cell-a") < 0 || (GetDouble("cell-b")<0 || GetDouble("cell-c")<0))
  {
   std::cerr << "Errors in cell longs. Check the input option in the system file." << '\n';
   return 1;
  }
  //Chequea entre los modulos la precencia de modulos "peligrosos" para casos como xyz y cell-a
  if(param["input-module"] == "xyz")
  {
   std::cerr << " WARNING : The XYZ module was loaded with 'cell a b c alpha beta gamma' " << '\n';
   std::cerr << " Please use vectors as input or ensure that the lattice vector build by lpmd are correct. " << '\n';
  }
/*  for (std::list<ModuleInfo>::iterator it=uselist.begin();it!=uselist.end();++it)
  {
   ModuleInfo & minf = *(it);
   std::cerr << minf.name << '\n';
   if (minf.name == "xyz")
   {
    std::cerr << " WARNING : The XYZ module was loaded with 'cell a b c alpha beta gamma' " << '\n';
    std::cerr << " Please use vectors as input or ensure that the lattice vector build by lpmd are correct. " << '\n';
   }
  }
  */
 }
 return 0;
}


