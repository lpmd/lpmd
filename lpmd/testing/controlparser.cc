/*
 *
 *
 *
 */

#include <lpmd/array.h>
#include <lpmd/util.h>
#include <lpmd/property.h>
#include "controlparser.h"

class UControlImpl
{
 public:
   PluginManager * pluginmanager;
   Array<ModuleInfo> plugins;
   Array<std::string> pluginpath;
   ParamList potentials;
   ParamList bonds;
   std::map<std::string, int> modulecounter;
   int implfilter_count;
};

UtilityControl::UtilityControl(PluginManager & pm)
{
 impl = new UControlImpl();
 impl->pluginmanager = &pm;
 impl->modulecounter["input"] = 0;
 impl->modulecounter["output"] = 0;
 impl->modulecounter["prepare"] = 0;
 impl->implfilter_count = 0;
 DeclareStatement("plugindir", "dir");
 DeclareStatement("cell cubic", "a scale");
 DeclareStatement("cell crystal", "a b c alpha beta gamma scale");
 DeclareStatement("cell vector", "ax ay az bx by bz cx cy cz scale");
 DeclareStatement("periodic", "x y z");
 DeclareStatement("potential", "module a b");
 DeclareStatement("cellmanager", "module");
 DeclareStatement("set", "option value");
 DeclareStatement("bond", "a b length");
 DeclareStatement("mass", "group value");
 DeclareStatement("charge", "group value");
 DeclareStatement("monitor", "properties start end each output");
 DeclareStatement("average", "properties interval start end each output");

 //
 DeclareBlock("use", "enduse");

 // 
 ParamList & params = (*this);
 params["periodic-x"] = "true";
 params["periodic-y"] = "true";
 params["periodic-z"] = "true";
 params["cell-scale"] = "1.0";
 params["replacecell"] = "false";
 params["optimize-simulation"] = "true";
 params["filter-end"] = "false";
}

UtilityControl::~UtilityControl() { delete impl; }

void UtilityControl::Read(const std::string & filename, const ParamList & options)
{
 ControlFile::Read(filename, options);
 if (bool((*this)["replacecell"])) (*this)["cell-type"] = "automatic";
}

void UtilityControl::Read(std::istream & istr, const ParamList & options, const std::string & filename)
{
 ControlFile::Read(istr, options, filename);
 if (bool((*this)["replacecell"])) (*this)["cell-type"] = "automatic";
}

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
 else if (name == "mass")
    massgroups[params["mass-group"]] = params["mass-value"];
 else if (name == "charge")
    chargegroups[params["charge-group"]] = params["charge-value"];
 else if ((name == "monitor") || (name == "average"))
 {
  std::string line;
  line = "visualize "+name+" start="+params[name+"-start"]+" ";
  line += ("end="+params[name+"-end"]+" each="+params[name+"-each"]+"\n");
  if (params[name+"-output"] == "") params[name+"-output"] = "-";
  
  ModuleInfo module_info("", "", ""); // FIXME
  module_info.name = name;
  module_info.id = name;
  module_info.args = ("properties "+params[name+"-properties"]+" output "+params[name+"-output"]);
  (impl->pluginmanager)->LoadPlugin(module_info);
  impl->plugins.Append(module_info);
  ReadLine(line);
 }
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
 else if (name == "filter")
 {
  Array<std::string> st_words = StringSplit(full_statement);
  int over_pos = st_words.Find("over");
  if (over_pos > 0)
  {
   std::string main_statement = "";
   for (int i=0;i<over_pos;++i) main_statement += (st_words[i]+" ");
   main_statement += ("filterby=implicitfilter"+ToString(impl->implfilter_count));
   std::string impl_filter = ("implicitfilter id=implicitfilter"+ToString(impl->implfilter_count)+" module=");
   for (int i=over_pos+1;i<st_words.Size();++i) impl_filter += (st_words[i]+" ");
   impl->implfilter_count++;
   ReadLine(impl_filter);
   ReadLine(main_statement);
   return 0;
  }
  else
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
 }
 else if (name == "implicitfilter")
 {
  std::string tmp = ParseCommandArguments(params, name, "id module ");
  std::string validkeywords = (impl->pluginmanager)->GetPluginKeywords(params[name+"-module"]);
  std::string args = ParseCommandArguments(params, name, validkeywords);
 
  module_info.name = params[name+"-module"];
  module_info.id = params[name+"-id"];
  module_info.args = args;

  // Load the plugin
  (impl->pluginmanager)->LoadPlugin(module_info);
  //
  impl->plugins.Append(module_info);

  params[name+"-modules"] += (params[name+"-module"]+" ");
  params.Remove(name+"-module");
  return 0;
 }
 else if (((name == "property") || (name == "apply")) || (name == "visualize"))
 {
  Array<std::string> st_words = StringSplit(full_statement);
  int over_pos = st_words.Find("over");
  if (over_pos > 0)
  {
   std::string main_statement = "";
   for (int i=0;i<over_pos;++i) main_statement += (st_words[i]+" ");
   main_statement += ("filterby=implicitfilter"+ToString(impl->implfilter_count));
   std::string impl_filter = ("implicitfilter id=implicitfilter"+ToString(impl->implfilter_count)+" module=");
   for (int i=over_pos+1;i<st_words.Size();++i) impl_filter += (st_words[i]+" ");
   impl->implfilter_count++;
   ReadLine(impl_filter);
   ReadLine(main_statement);
   return 0;
  }
  else
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
     Plugin & plugin = (*(impl->pluginmanager))[impl->plugins[q].id];
     if (int(plugin["each"]) == 0)
     {
      plugin["start"] = "0";
      plugin["end"] = "-1";
      plugin["each"] = "1";
     }
    }
   }
   params[name+"-modules"] += (params[name+"-module"]+" ");
   params.Remove(name+"-module");
   return 0;
  }
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
 else return 1;
 return 0;
}

Array<ModuleInfo> & UtilityControl::Plugins() const { return impl->plugins; }

Array<std::string> & UtilityControl::PluginPath() const { return impl->pluginpath; }

ParamList & UtilityControl::Potentials() const { return impl->potentials; } 

ParamList & UtilityControl::Bonds() const { return impl->bonds; } 

