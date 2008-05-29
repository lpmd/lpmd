//
//
//

#include "input.h"

#include <lpmd/util.h>
#include <iostream>
#include <sstream>
#include <cassert>

using namespace lpmd;

//
//
//
CommonInputReader::CommonInputReader(PluginManager & pm): ovpointer(NULL)
{ 
 pluginman = &pm;
 DeclareStatement("use", "module as alias");
 DeclareStatement("plugindir", "dir");
 DeclareStatement("include", "inputfile");
 DeclareStatement("cell cubic", "a scale");
 DeclareStatement("cell crystal", "a b c alpha beta gamma scale");
 DeclareStatement("cell vector", "ax ay az bx by bz cx cy cz scale");
 DeclareStatement("periodic", "x y z");
 DeclareStatement("potential", "module a b");
 DeclareStatement("cellmanager", "module");
 DeclareStatement("set", "option value");

 // Some default values
 ParamList & param = (*this);
 param["cell-scale"] = "1.0";
 param["periodic-x"] = "true";
 param["periodic-y"] = "true";
 param["periodic-z"] = "true";
 param["distancecache"] = "false";
 param["property-list"] = "";
 param["apply-list"] = "";
}

CommonInputReader::~CommonInputReader() { }

//
//
//
void CommonInputReader::Read(std::string sysfile, const ParamList & optionvars)
{
 inside_useblock = false;
 ovpointer = &optionvars;
 InputFile::Read(sysfile, optionvars);
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
int CommonInputReader::OnStatement(const std::string & name, const std::string & keywords, bool regular)
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
  else if (name == "include")
  {
   assert(ovpointer != NULL);
   Read(param["include-inputfile"], *ovpointer);
  }
  else if (name == "set") { param.AssignParameter(param["set-option"], param["set-value"]); }
  else if (name == "plugindir") pm.AddToPluginPath(param["plugindir-dir"]);
  else if (name == "potential") 
     param["potential-list"] = param["potential-list"]+param["potential-a"]+"#"+param["potential-b"]+"#"+param["potential-module"]+" ";
  else if (name == "charge") param["charge-"+param["charge-symbol"]] = param["charge-charge"];
  else if (((name == "property") || (name == "apply")) || (name == "visualize"))
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
  else if (name == "input")
  {
   std::list<std::string> tmpwords = words;
   std::string validkws = "module ";
   ParseCommandArguments(name, validkws);
   std::string pluginkws = pm.GetPluginKeywords(param[name+"-module"]);
   words = tmpwords;
   std::string keyw = ParseCommandArguments(name, validkws+pluginkws);
   param[name+"-moduleargs"] = keyw;
  }
  else if ((name == "prepare") || (name == "output"))
  {
   std::list<std::string> tmpwords = words;
   std::string validkws = "module ";
   ParseCommandArguments(name, validkws);
   std::string pluginkws = pm.GetPluginKeywords(param[name+"-module"]);
   words = tmpwords;
   std::string keyw = ParseCommandArguments(name, validkws+pluginkws);
   param[name+"-moduleargs"] = keyw;
   std::ostringstream ostr;
   std::list<ModuleInfo> * taglist;
   if (name == "prepare") taglist = &preparelist;
   else taglist = &outputlist;
   ostr << taglist->size();
   ModuleInfo minf(param[name+"-module"], name+ostr.str(), param[name+"-moduleargs"]);
   taglist->push_back(minf);
  }
  else return 1; // Unexpected statement
 }
 return 0; // status OK
}

//
//
//
int CommonInputReader::CheckConsistency()
{
 ParamList & param = (*this);
 /*
  * Check propiedades de la Celda :
  */
 if (param.Defined("cell-a"))
 {
  if(GetDouble("cell-a") < 0 || (GetDouble("cell-b")<0 || GetDouble("cell-c")<0))
    EndWithError("Errors in cell lengths. Please check the \"cell\" statement in the input file.");
  //Chequea entre los modulos la precencia de modulos "peligrosos" para casos como xyz y cell-a
  if (param["input-module"] == "xyz")
  {
   ShowWarning("input file", "The xyz module was loaded with 'cell a b c alpha beta gamma'\nPlease use vectors as input or make sure that the lattice vectors built by lpmd are correct.");
  }
 }
 return 0;
}


