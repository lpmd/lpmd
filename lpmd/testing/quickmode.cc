/*
 *
 *
 *
 */

#include "quickmode.h"
#include <lpmd/error.h>

class QuickModeImpl
{
 public:
  std::string autouse_statement;
};

QuickModeParser::QuickModeParser(const std::string & use_hint)
{
 impl = new QuickModeImpl();
 impl->autouse_statement = use_hint;
 DefineOption("help", "h", "");
 DefineOption("verbose", "v", "");
 DefineOption("pluginhelp", "p", "name");
 DefineOption("option", "O", "keywordvalue");
 DefineOption("replace-cell", "r", "");
 DefineOption("lengths", "L", "a,b,c");
 DefineOption("angles", "A", "alpha,beta,gamma");
 DefineOption("vector", "V", "ax,ay,az,bx,by,bz,cx,cy,cz");
 DefineOption("scale", "S", "value");
 DefineOption("input", "i", "options");
 DefineOption("output", "o", "options");
 DefineOption("use", "u", "options");
 DefineOption("cellmanager", "c", "options");
 DefineOption("test-plugin", "T", "name");
}

QuickModeParser::~QuickModeParser() { delete impl; }

Array<std::string> ParseModuleFlags(const std::string & optflags)
{
 Array<std::string> opt = StringSplit(optflags, ':');
 return StringSplit(opt[1], ',');
}

std::string ParseModuleLoading(const std::string & optflags)
{
 Array<std::string> opt = StringSplit(optflags, ':');
 std::string modname = opt[0];
 Array<std::string> flags = ParseModuleFlags(optflags);
 std::string joinedflags = "";
 for (int q=0;q<flags.Size();++q) joinedflags += (flags[q]+" ");
 RemoveUnnecessarySpaces(joinedflags);
 return ("module="+modname+" "+joinedflags+"\n");
}

std::string ParseUseOptions(const std::string optflags, const std::string use_hint)
{
 std::string control = "";
 Array<std::string> opt = StringSplit(optflags, ':');
 std::string modname = opt[0];
 Array<std::string> flags = ParseModuleFlags(optflags);
 control += ("use "+modname+"\n");
 for (int q=0;q<flags.Size();++q) 
 {
  Array<std::string> pair = StringSplit(flags[q], '=');
  control += (pair[0]+" "+pair[1]+"\n");
 }
 control += ("enduse\n"+use_hint+" "+modname+"\n");
 return control;
}

std::string QuickModeParser::FormattedAsControlFile() const
{
 std::string control = "";
 std::string cellstatement = "";
 std::string cell_scale = "1.0";
 if (Defined("pluginhelp-name")) control += "use "+(*this)["pluginhelp-name"]+" as help_plugin\nenduse\n";
 if (Defined("verbose")) control += "set verbose true\n";
 if (Defined("replace-cell")) control += "set replacecell true\n";
 if (Defined("option-keywordvalue"))
 {
  Array<std::string> options = StringSplit((*this)["option-keywordvalue"], ',');
  for (int p=0;p<options.Size();++p)
  {
   Array<std::string> pair = StringSplit(options[p], '=');
   if (pair.Size() != 2) throw SyntaxError("Setting options with -O");
   std::string key = pair[0];
   std::string value = pair[1];
   control += ("set "+key+" "+value+"\n");
  }
 } 
 //
 if (Defined("vector"))
 {
  Array<std::string> comp = StringSplit((*this)["vector-ax,ay,az,bx,by,bz,cx,cy,cz"], ',');
  if (comp.Size() != 9) throw SyntaxError("Only nine vector components are supported with -V");
  cellstatement = "cell vector ax="+comp[0]+" ay="+comp[1]+" az="+comp[2];
  cellstatement += (" bx="+comp[3]+" by="+comp[4]+" bz="+comp[5]+" cx="+comp[6]+" cy="+comp[7]+" cz="+comp[8]);
 }
 else 
 {
  if (Defined("lengths"))
  {
   Array<std::string> box = StringSplit((*this)["lengths-a,b,c"], ',');
   if (box.Size() == 1) cellstatement = "cell cubic a="+box[0];
   else if (box.Size() == 3) cellstatement = "cell crystal a="+box[0]+" b="+box[1]+" c="+box[2];
   else throw SyntaxError("Only one or three arguments are supported with -L");
  }
  if (Defined("angles"))
  {
   Array<std::string> angles = StringSplit((*this)["angles-alpha,beta,gamma"], ',');
   if (angles.Size() == 3) cellstatement += " alpha="+angles[0]+" beta="+angles[1]+" gamma="+angles[2];
   else throw SyntaxError("Only three arguments are supported with -A");
  }
 }
 if (Defined("scale-value")) cell_scale = (*this)["scale-value"];
 if (cellstatement != "") control += (cellstatement+" scale="+cell_scale+"\n");
 if (Defined("input-options")) control += ("input "+ParseModuleLoading((*this)["input-options"]));
 if (Defined("output-options")) control += ("output "+ParseModuleLoading((*this)["output-options"]));
 if (Defined("use-options")) control += ParseUseOptions((*this)["use-options"], impl->autouse_statement);
 if (Defined("cellmanager-options"))
 {
  std::string optflags = (*this)["cellmanager-options"];
  Array<std::string> opt = StringSplit(optflags, ':');
  std::string modname = opt[0];
  control += (ParseUseOptions((*this)["cellmanager-options"], "cellmanager"));
 }
 return control;
}


