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
   Array<ModuleInfo> plugins;
   Array<TypeInfo> types;
   Array<std::string> pluginpath;
   ParamList potentials;
   ParamList bonds;
};

UtilityControl::UtilityControl()
{
 impl = new UControlImpl();
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
}

UtilityControl::~UtilityControl() { delete impl; }

int UtilityControl::OnRegularStatement(const std::string & name, const ParamList & keywords)
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

 return 0;
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

