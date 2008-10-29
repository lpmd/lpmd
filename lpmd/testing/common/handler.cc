//
//
//

#include "version.h"
#include "handler.h"
#include "cmdline.h"
#include "config.h"

#include <lpmd/util.h>
#include <lpmd/potentialarray.h>
#include <lpmd/simulationcell.h>
#include <lpmd/cellformat.h>
#include <lpmd/cellmanager.h>

#include <iostream>
#include <sstream>

using namespace lpmd;

//
//
//

CommonHandler::CommonHandler(const std::string & cmdnam, const std::string & fullnam): cmdname(cmdnam), name(fullnam), parm(NULL) 
{ 

}

CommonHandler::~CommonHandler() { delete scell; }

void CommonHandler::BannerPrint(std::string text)
{
 std::cout << std::setfill('*');
 std::cout << std::endl;
 std::cout << std::setw(80) << "" << std::endl;
 std::cout << "* " << std::setw(78) << std::left << text+" " << std::endl;
 std::cout << std::setw(80) << "" << std::endl;
 std::cout << std::setfill(' ');
}

void CommonHandler::LoadUseModules()
{
 //
 // Load each of the modules called with "use", passing its parameters
 //
 CommonInputReader & param = GetInputReader();
 for (std::list<ModuleInfo>::const_iterator it=param.uselist.begin();it != param.uselist.end();++it)
 {
  const ModuleInfo & minf = *it;
  pluginman.LoadPlugin(minf.name, minf.id, minf.args);
 }
}

void CommonHandler::LoadModules()
{
 PotentialArray & p_array = GetPotentialArray();
 CommonInputReader & param = GetInputReader();
 std::list<std::string> ptlist = ListOfTokens(param["potential-list"]);
 for (std::list<std::string>::const_iterator it=ptlist.begin();it!=ptlist.end();++it)
 {
  std::list<std::string> tmp = ListOfTokens(*it, '#');
  const std::string sp1 = tmp.front();
  tmp.pop_front();
  const std::string sp2 = tmp.front();
  tmp.pop_front();
  const std::string potname = tmp.front();
  tmp.pop_front();
  Potential & pot = CastModule<Potential>(pluginman[potname]);
  pluginman[potname].SetUsed();
  p_array.Set(sp1, sp2, pot);
 }
}

void CommonHandler::ShowConfigsInfo(std::vector<SimulationCell> & configs)
{
 // Muestra informacion util acerca de las configuraciones
 std::cerr << '\n';
 for (unsigned long int i=0;i<configs.size();++i)
 {
  SimulationCell & sc = configs[i];
  Vector pos, cm(0.0, 0.0, 0.0);
  double tmass = 0.0, xmin, xmax, ymin, ymax, zmin, zmax;
  xmin = xmax = sc[0].Position().GetX();
  ymin = ymax = sc[0].Position().GetY();
  zmin = zmax = sc[0].Position().GetZ();
  for (long int j=0;j<sc.Size();++j)
  {
   pos = sc[j].Position();
   cm = cm + sc[j].Mass()*pos;
   tmass += sc[j].Mass();
   if (pos.GetX() < xmin) xmin = pos.GetX();
   if (pos.GetX() > xmax) xmax = pos.GetX();
   if (pos.GetY() < ymin) ymin = pos.GetY();
   if (pos.GetY() > ymax) ymax = pos.GetY();
   if (pos.GetZ() < zmin) zmin = pos.GetZ();
   if (pos.GetZ() > zmax) zmax = pos.GetZ();
  }
  cm = cm / tmass;
  std::cerr << "Configuration " << i << ": \n";
  std::cerr << "-> Number of atoms  : " << sc.Size() << " atoms\n";
  for (int q=0;q<3;++q) 
     std::cerr << "-> h[" << q << "]             : " << sc.GetVector(q) << '\n';
  std::cerr << "-> Volume           : " << sc.Volume() << '\n'; 
  std::cerr << "-> Center of Mass   : " << cm << '\n'; 
  std::cerr << "-> X coordinate     : [ " << xmin << " , " << xmax << " ] (width: " << xmax-xmin << ")\n";
  std::cerr << "-> Y coordinate     : [ " << ymin << " , " << ymax << " ] (width: " << ymax-ymin << ")\n";
  std::cerr << "-> Z coordinate     : [ " << zmin << " , " << zmax << " ] (width: " << zmax-zmin << ")\n";
  std::cerr << "-> X displ. from CM : [ " << xmin-cm.Get(0) << " , " << xmax-cm.Get(0) << " ]\n";
  std::cerr << "-> Y displ. from CM : [ " << ymin-cm.Get(1) << " , " << ymax-cm.Get(1) << " ]\n";
  std::cerr << "-> Z displ. from CM : [ " << zmin-cm.Get(2) << " , " << zmax-cm.Get(2) << " ]\n";
  std::cerr << '\n';
 }
}

//
//
//
void CommonHandler::Initialize()
{
 Cell *cell = NULL;
 // Construye la celda de simulacion
 CommonInputReader & param = GetInputReader();
 if (param.Defined("cell-a") && !param.Defined("cell-alpha"))
 {
  double a, scale;
  scale = param.GetDouble("cell-scale");
  a = param.GetDouble("cell-a")*scale;
  cell = new Cell(a, a, a, 0.5*M_PI, 0.5*M_PI, 0.5*M_PI);
 }
 else if (param.Defined("cell-alpha") && param.Defined("cell-a"))
 {
  double a, b, c, alpha, beta, gamma, scale;
  scale = param.GetDouble("cell-scale");
  a = param.GetDouble("cell-a")*scale;
  b = param.GetDouble("cell-b")*scale;
  c = param.GetDouble("cell-c")*scale;
  alpha = param.GetDouble("cell-alpha")*M_PI/180.0e0;
  beta = param.GetDouble("cell-beta")*M_PI/180.0e0;
  gamma = param.GetDouble("cell-gamma")*M_PI/180.0e0;
  cell = new Cell(a, b, c, alpha, beta, gamma);
 }
 else if (param.Defined("cell-ax"))
 {
  double scale = param.GetDouble("cell-scale");
  Vector a(param.GetDouble("cell-ax")*scale, param.GetDouble("cell-ay")*scale, param.GetDouble("cell-az")*scale);
  Vector b(param.GetDouble("cell-bx")*scale, param.GetDouble("cell-by")*scale, param.GetDouble("cell-bz")*scale);
  Vector c(param.GetDouble("cell-cx")*scale, param.GetDouble("cell-cy")*scale, param.GetDouble("cell-cz")*scale);
  cell = new Cell(a, b, c);
 }
 else
 {
  if (param["replacecell"] == "true") cell = new Cell(1.0, 1.0, 1.0); 
  else EndWithError("The dimensions of the simulation cell were not specified. Maybe you forgot a \"cell\" statement?"); 
 }

 bool px, py, pz;
 px = param.GetBool("periodic-x");
 py = param.GetBool("periodic-y");
 pz = param.GetBool("periodic-z");
 
 scell = new SimulationCell(1, 1, 1, px, py, pz);
 scell->SetCell(*cell);
 SetCell(*scell);
 delete cell;

 if (Verbose() && (! param.GetBool("replacecell")))
 {
  std::cerr << "Cell vectors: " << '\n';
  for (int k=0;k<3;++k) std::cerr << GetCell().GetVector(k) << '\n';
  std::cerr << "Volume: " << GetCell().Volume() << '\n';
 }

 if (param["restore-dumpfile"] == "")
 {
  //
  // This loads the input (cell generator) module: 
  //
  if (param["input-module"] == "") EndWithError("A CellGenerator plugin was not assigned. Maybe you forgot an \"input\" statement?");
  else
  {
   std::string extra_args = "";
   if (param.GetBool("replacecell")) extra_args = "replacecell true ";
   pluginman.LoadPlugin(param["input-module"], extra_args+param["input-moduleargs"]);
   pluginman[param["input-module"]].SetUsed();
  }
 }
 else { } // modo restore, input-module esta vacio
}

void CommonHandler::ShowHelp()
{
 std::cerr << name << " version " << VERSION;
 #ifndef NUMBERED_RELEASE
 std::cerr << " (from " << SVNBRANCH << ", revision " << SVNREVISION << ")";
 #endif
 std::cerr << '\n';
 std::cerr << "Using liblpmd version " << lpmd::LibraryVersion() << std::endl << std::endl;
 std::cerr << "Usage: " << cmdname << " [--verbose | -v ] [--lengths | -L <a,b,c>] [--angles | -A <alpha,beta,gamma>]";
 std::cerr << " [--vector | -V <ax,ay,az,bx,by,bz,cx,cy,cz>] [--scale | -S <value>]";
 std::cerr << " [--option | -O <option=value,option=value,...>] [--input | -i plugin:opt1,opt2,...] [--output | -o plugin:opt1,opt2,...]";
 std::cerr << " [--use | -u plugin:opt1,opt2,...] [--cellmanager | -c plugin:opt1,opt2,...] [--replace-cell | -r] [file.control]\n";
 std::cerr << "       " << cmdname << " [--pluginhelp | -p <pluginname>]\n";
 std::cerr << "       " << cmdname << " [--help | -h]\n";
 exit(1);
}

void CommonHandler::ShowPluginHelp(std::string plugin)
{
 std::ostringstream str;
 str << "Show "<<plugin<<" Plugin Information.";
 BannerPrint(str.str());
 PluginManager pm;
 pm.LoadPlugin(plugin, "tempmodule", "");
 std::cout << "Loaded from file: " << pm["tempmodule"]["fullpath"] << '\n' << '\n';
 pm["tempmodule"].ShowHelp();
 BannerPrint("Provides");
 std::cout << "     >> " << pm["tempmodule"].Provides() << '\n';
 BannerPrint("Arguments Required or supported");
 std::cout << "     >> " << pm["tempmodule"].Keywords() << '\n';
 BannerPrint("Default values for parameters");
 pm["tempmodule"].Show(std::cout);
 exit(1);
}

void CommonHandler::SetOptionVariables(CommonCmdLineParser & clp, ParamList & ov)
{
 if (clp.Defined("option"))
 {
  std::list<std::string> kwlist = ListOfTokens(clp["option-keywordvalue"], ',');
  for (std::list<std::string>::const_iterator it=kwlist.begin();it!=kwlist.end();++it)
  {
   std::vector<std::string> kwpair = SplitTextLine(*it, '=');
   ov.AssignParameter(kwpair[0], kwpair[1]);
  } 
 }
}

//
// Main Program
//
void CommonHandler::Execute(CommonCmdLineParser & clp)
{
 std::list<std::string> args = clp.Arguments();
 if ((args.size() == 1) && ((! clp.Defined("input")) && (! clp.Defined("pluginhelp")))) ShowHelp();
 args.pop_front();
 if (args.size() == 1) { clp.AssignParameter("inputfile-file", args.front()); }
 SetVerbose(false);
 if (clp.Defined("pluginhelp")) ShowPluginHelp(clp["pluginhelp-file"]);
 if (clp.Defined("help")) ShowHelp();
 if (clp.Defined("verbose")) SetVerbose(true);
 ParamList optionvars;
 SetOptionVariables(clp, optionvars); 
 CommonInputReader & param = GetInputReader();
 if (clp.Defined("input"))
 {
  std::string inpline = "input ";
  std::vector<std::string> ospl = SplitTextLine(clp["input-options"], ':');
  std::vector<std::string> args = SplitTextLine(ospl[1], ',');
  inpline += (ospl[0]+" ");
  for (unsigned int i=0;i<args.size();++i) inpline += (args[i]+" ");
  param.ParseLine(inpline);
 }
 if (clp.Defined("output"))
 {
  std::string outline = "output ";
  std::vector<std::string> ospl = SplitTextLine(clp["output-options"], ':');
  std::vector<std::string> args = SplitTextLine(ospl[1], ',');
  outline += (ospl[0]+" ");
  for (unsigned int i=0;i<args.size();++i) outline += (args[i]+" ");
  param.ParseLine(outline);
 }
 if (clp.Defined("use"))
 {
  std::vector<std::string> ospl = SplitTextLine(clp["use-options"], ':');
  std::vector<std::string> args;
  if (ospl.size() > 1) args = SplitTextLine(ospl[1], ',');
  std::string useargs = "";
  for (unsigned int i=0;i<args.size();++i)
  {
   std::vector<std::string> tmp = SplitTextLine(args[i], '=');
   useargs += (tmp[0]+" "+tmp[1]+" ");
  }
  ModuleInfo minf(ospl[0], ospl[0], useargs);
  param.uselist.push_back(minf);
  param["special-list"] = param["special-list"]+ospl[0]+" ";
 }
 if (clp.Defined("cellmanager"))
 {
  std::string outline = "cellmanager ";
  std::vector<std::string> cspl = SplitTextLine(clp["cellmanager-options"], ':');
  std::vector<std::string> args;
  if (cspl.size() > 1) args = SplitTextLine(cspl[1], ',');
  outline += (cspl[0]+" ");
  std::string useargs = "";
  for (unsigned int i=0;i<args.size();++i)
  {
   std::vector<std::string> tmp = SplitTextLine(args[i], '=');
   useargs += (tmp[0]+" "+tmp[1]+" ");
  }
  ModuleInfo minf(cspl[0], cspl[0], useargs);
  param.uselist.push_back(minf);
  param["cellmanager-module"] = cspl[0];
  param.ParseLine(outline); 
 }
 if (clp["inputfile-file"] != "")
 {
  if (Verbose()) std::cerr << "-> Reading input file: \"" << clp["inputfile-file"] << "\"\n";
  param.Read(clp["inputfile-file"], optionvars);
 }
 if (clp.Defined("replace-cell")) param["replacecell"] = "true";
 if (clp.Defined("vector"))
 {
  if (clp.Defined("lengths") || clp.Defined("angles")) 
     EndWithError("Option -V (--vector) is mutually exclusive with -L (--lengths) and -A (--angles)");
  std::vector<std::string> vv = SplitTextLine(clp["vector-ax,ay,az,bx,by,bz,cx,cy,cz"], ',');
  param.AssignParameter("cell-ax", vv[0]);
  param.AssignParameter("cell-ay", vv[1]);
  param.AssignParameter("cell-az", vv[2]);
  param.AssignParameter("cell-bx", vv[3]);
  param.AssignParameter("cell-by", vv[4]);
  param.AssignParameter("cell-bz", vv[5]);
  param.AssignParameter("cell-cx", vv[6]);
  param.AssignParameter("cell-cy", vv[7]);
  param.AssignParameter("cell-cz", vv[8]);
 }
 if (clp.Defined("lengths"))
 {
  if (!clp.Defined("angles")) 
  {
   param.AssignParameter("cell-alpha", "90.0");
   param.AssignParameter("cell-beta", "90.0");
   param.AssignParameter("cell-gamma", "90.0");
  }
  std::vector<std::string> ll = SplitTextLine(clp["lengths-a,b,c"], ',');
  param.AssignParameter("cell-a", ll[0]);
  if (ll.size() > 1)
  {
   param.AssignParameter("cell-b", ll[1]);
   param.AssignParameter("cell-c", ll[2]);
  }
  else
  {
   param.AssignParameter("cell-b", ll[0]);
   param.AssignParameter("cell-c", ll[0]);
  }
 }
 if (clp.Defined("angles"))
 {
  if (!clp.Defined("lengths")) EndWithError("Option -A (--angles) must be used together with -L (--lengths)");
  std::vector<std::string> aa = SplitTextLine(clp["angles-alpha,beta,gamma"], ',');
  param.AssignParameter("cell-alpha", aa[0]);
  param.AssignParameter("cell-beta", aa[1]);
  param.AssignParameter("cell-gamma", aa[2]);
 }
 if (clp.Defined("scale")) param.AssignParameter("cell-scale", clp["scale-value"]);
 if (Verbose()) std::cerr << "-> Input file after parsing: \n" << GetInputReader() << '\n';
 int status = GetInputReader().CheckConsistency();
 if (status != 0) { EndWithError("Input file \" " + clp["inputfile-file"] + "\" returned an error"); }
 LoadUseModules();
 Initialize();
 LoadModules();
 Process();
 Finish();
}

