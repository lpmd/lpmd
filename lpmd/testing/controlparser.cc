/*
 *
 *
 *
 */

#include <lpmd/array.h>
#include <lpmd/util.h>
#include "controlparser.h"

class UControlImpl
{
 public:
   PluginManager * pluginmanager;
   Array<ModuleInfo> plugins;
   Array<TypeInfo> types;
   Array<std::string> pluginpath;
   ParamList potentials;
   ParamList bonds;
   std::map<std::string, int> modulecounter;
};

UtilityControl::UtilityControl(PluginManager & pm)
{
 impl = new UControlImpl();
 impl->pluginmanager = &pm;
 impl->modulecounter["input"] = 0;
 impl->modulecounter["output"] = 0;
 impl->modulecounter["prepare"] = 0;
 DeclareStatement("plugindir", "dir");
 DeclareStatement("cell cubic", "a scale");
 DeclareStatement("cell crystal", "a b c alpha beta gamma scale");
 DeclareStatement("cell vector", "ax ay az bx by bz cx cy cz scale");
 DeclareStatement("periodic", "x y z");
 DeclareStatement("potential", "module a b");
 DeclareStatement("cellmanager", "module");
 DeclareStatement("set", "option value");
 DeclareStatement("bond", "a b length");

 //
 DeclareBlock("use", "enduse");
 DeclareBlock("type", "endtype");

 // 
 ParamList & params = (*this);
 params["periodic-x"] = "true";
 params["periodic-y"] = "true";
 params["periodic-z"] = "true";
 params["cell-scale"] = "1.0";
}

UtilityControl::~UtilityControl() { delete impl; }

int UtilityControl::OnRegularStatement(const std::string & name, const std::string & keywords)
{
 ParamList & params = (*this);
 if (name == "cell")
 {
  if ((! params.Defined("cell-b")) && (! params.Defined("cell-ay"))) params["cell-type"] = "cubic";
  else if (params.Defined("cell-b")) params["cell-type"] = "crystal";
  else if (params.Defined("cell-ax")) params["cell-type"] = "vector";
 }
 else if (name == "set")
 {
  params[params["set-option"]] = params["set-value"];
  params.Remove("set-option");
  params.Remove("set-value");
 }
 else if (name == "potential")
 {
  impl->potentials[params["potential-a"]+"-"+params["potential-b"]] = params["potential-module"];
 }
 else if (name == "bond")
 {
  impl->bonds[params["bond-a"]+"-"+params["bond-b"]] = params["bond-length"];
 }
 else if (name == "plugindir") impl->pluginpath.Append(params["plugindir-dir"]);
 return 0;
}

int UtilityControl::OnNonRegularStatement(const std::string & name, const std::string & full_statement)
{
 Map & params = (*this);
 ModuleInfo module_info("", "", ""); //FIXME: mejor constructor!
 if (((name == "input") || (name == "prepare")) || (name == "output"))
 {
  impl->modulecounter[name]++;
  std::string tmp = ParseCommandArguments(params, name, "module ");
  std::string validkeywords = (impl->pluginmanager)->GetPluginKeywords(params[name+"-module"]);
  std::string args = ParseCommandArguments(params, name, validkeywords);
  params[name+"-modules"] += (params[name+"-module"]+" ");
  module_info.name = params[name+"-module"];
  module_info.id = name+ToString(impl->modulecounter[name]);
  module_info.args = args;

  // Load the plugin
  (impl->pluginmanager)->LoadPlugin(module_info);
  //
  impl->plugins.Append(module_info);
  params.Remove(name+"-module");
  return 0;
 }
 else if (((name == "property") || (name == "apply")) || (name == "visualize"))
 {
  std::string tmp = ParseCommandArguments(params, name, "module ");
  std::string validkeywords = (impl->pluginmanager)->GetPluginKeywords(params[name+"-module"]);
  std::string args = ParseCommandArguments(params, name, validkeywords);
  for (int q=0;q<impl->plugins.Size();++q)
  {
   if (impl->plugins[q].id == params[name+"-module"]) 
   {
    impl->plugins[q].args += (" "+args);
    (impl->pluginmanager)->UpdatePlugin(params[name+"-module"], impl->plugins[q].args); 
   }
  }
  params[name+"-modules"] += (params[name+"-module"]+" ");
  params.Remove(name+"-module");
  return 0;
 }
 return 1;
}

int UtilityControl::OnBlock(const std::string & name, const std::string & full_statement)
{
 if (name == "use")
 {
  ModuleInfo module_info("", "", ""); // FIXME: mejor constructor!
  Array<std::string> tokens = StringSplit(full_statement);
  assert(tokens[0] == "useblock");
  module_info.name = tokens[1];
  module_info.id = module_info.name;
  int pstart = 2;
  if ((tokens.Size() > 2) && (tokens[2] == "as")) 
  { 
   module_info.id = tokens[3];
   pstart = 4;
  }
  for (int p=pstart;p<tokens.Size();++p) module_info.args += (tokens[p]+" ");
  RemoveUnnecessarySpaces(module_info.args);
  // Load the plugin
  (impl->pluginmanager)->LoadPlugin(module_info);
  //
  impl->plugins.Append(module_info);
 }
 else if (name == "type")
 {
  TypeInfo type_info;
  Array<std::string> tokens = StringSplit(full_statement);
  assert(tokens[0] == "typeblock");
  type_info.name = tokens[1];
  tokens.Delete(0);
  tokens.Delete(0);
  while (tokens.Size() > 0)
  {
   type_info[tokens[0]] = tokens[1];
   tokens.Delete(0);
   tokens.Delete(0);
  }
  impl->types.Append(type_info);
 }
 else return 1;
 return 0;
}

Array<ModuleInfo> & UtilityControl::Plugins() const { return impl->plugins; }

Array<TypeInfo> & UtilityControl::Types() const { return impl->types; }
  
Array<std::string> & UtilityControl::PluginPath() const { return impl->pluginpath; }

ParamList & UtilityControl::Potentials() const { return impl->potentials; } 

ParamList & UtilityControl::Bonds() const { return impl->bonds; } 

