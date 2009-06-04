//
//
//

#include "handler.h"
#include "cmdline.h"
#include "config.h"

#include <lpmd/util.h>
#include <lpmd/session.h>
#include <lpmd/potentialarray.h>
#include <lpmd/simulationcell.h>
#include <lpmd/cellformat.h>
#include <lpmd/cellmanager.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <unistd.h>

using namespace lpmd;

//
//
//

CommonHandler::CommonHandler(const std::string & cmdnam, const std::string & fullnam): cmdname(cmdnam), name(fullnam), parm(NULL) 
{ 

}

CommonHandler::~CommonHandler() { delete scell; }

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
  if (param["replacecell"] == "true") cell = new Cell(e1, e2, e3); 
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
  const SimulationCell & sc = GetCell();
  const Cell & innercell = sc.GetCell();
  std::cerr << "Cell vectors: " << '\n';
  for (int k=0;k<3;++k) std::cerr << innercell[k] << '\n';
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

const std::string CommonHandler::ParseQuickModeOptions(const std::string & t, CommonCmdLineParser & clp)
{
 std::string tline = t+" ";
 std::vector<std::string> ospl = StringSplit< std::vector<std::string> >(clp[t+"-options"], ':');
 std::vector<std::string> args;
 if (ospl.size() > 1)
 {
  std::vector<std::string> values = FindBetween(ospl[1]);
  args = StringSplit< std::vector<std::string> >(ospl[1],',');
  for(unsigned int i=0;i<args.size();++i) 
  {
   size_t p;
   p = args[i].find_first_of("%");
   if (p!=std::string::npos)
   {
    std::string sub = args[i].substr(p,args[i].size()-p);
    int l=atoi(sub.c_str());
    args[i].replace(p,args[i].size()-p,values[l]);
   }
  }
 }
 tline += (ospl[0]+" ");
 long int s = StringSimpleHash(t+ospl[0]);
 if (s == 0xcc66) clp["-magic1"] = "true";
 if (s == 0xc57b) clp["-magic2"] = "true";
 if (s == 0xb848) clp["-magic3"] = "true";
 for (unsigned int i=0;i<args.size();++i) tline += (args[i]+" ");
 return tline;
}

//
// Main Program
//
void CommonHandler::Execute(CommonCmdLineParser & clp)
{
 std::list<std::string> args = clp.Arguments();
 if ((args.size() == 1) && ((! clp.Defined("input") && (! clp.Defined("output"))) && (! clp.Defined("pluginhelp")))) ShowHelp();
 args.pop_front();
 if (args.size() == 1) { clp.AssignParameter("inputfile-file", args.front()); }
 SetVerbose(false);
 if (clp.Defined("verbose")) SetVerbose(true);
 ParamList optionvars;
 ModuleInfo minf("dummyplugin", "dummyplugin", "");
 SetOptionVariables(clp, optionvars); 
 CommonInputReader & param = GetInputReader();
 if (clp.Defined("input")) param["input-genline"] = ParseQuickModeOptions("input", clp);
 if (clp.Defined("output")) param["output-genline"] = ParseQuickModeOptions("output", clp);
 if (clp.Defined("use"))
 {
  ParseQuickModeOptions("use", clp, minf);
  param.uselist.push_back(minf);
  param["special-list"] = param["special-list"]+minf.name+" ";
 }
 if (clp.Defined("cellmanager"))
 {
  std::string tmp = ParseQuickModeOptions("cellmanager", clp, minf);
  param.uselist.push_back(minf);
  param["cellmanager-module"] = minf.name;
  param.ParseLine(tmp); 
 }
 if (clp.Defined("pluginhelp")) ShowPluginHelp(clp["pluginhelp-file"]);
 if (clp.Defined("help")) ShowHelp();
 if ((StringSimpleHash(cmdname) == 0xb746) && (clp.Defined("-magic1")))
 {
  const char error1[] = {0x46,0x6f,0x72,0x20,0x74,0x68,0x61,0x74,0x20,0x49,0x27,0x6c,0x6c,0x20,0x6e,\
                        0x65,0x65,0x64,0x20,0x61,0x20,0x72,0x65,0x61,0x6c,0x6c,0x79,0x20,0x67,0x6f,\
                        0x6f,0x64,0x20,0x69,0x64,0x65,0x61,0x20,0x61,0x6e,0x64,0x20,0x61,0x20,0x6c,\
                        0x6f,0x74,0x20,0x6f,0x66,0x20,0x63,0x6f,0x66,0x66,0x65,0x65, 0x0};
  const char warn1[] = {0x53,0x6f,0x72,0x72,0x79,0x2c,0x20,0x49,0x20,0x63,0x61,0x6e,0x27,0x74,0x20,\
                        0x63,0x6f,0x6d,0x65,0x20,0x75,0x70,0x20,0x77,0x69,0x74,0x68,0x20,0x61,0x6e,\
                        0x79,0x74,0x68,0x69,0x6e,0x67,0x0};
  const char warn2[] = {0x47,0x72,0x65,0x61,0x74,0x21,0x20,0x4e,0x6f,0x77,0x20,0x74,0x68,0x65,0x20,\
                        0x63,0x6f,0x66,0x66,0x65,0x65,0x2e,0x2e,0x2e,0x0};
  const char mesg1[] = {0x41,0x6c,0x72,0x69,0x67,0x68,0x74,0x21,0x20,0x4c,0x65,0x74,0x20,0x6d,0x65,\
                        0x20,0x74,0x68,0x69,0x6e,0x6b,0x2e,0x2e,0x2e,0x20,0x0};
  const char mesg2[] = {0xa,0x4f,0x4b,0x2c,0x20,0x48,0x65,0x72,0x65,0x20,0x79,0x6f,0x75,0x20,\
                        0x67,0x6f,0x2e,0x2e,0x2e,0xa,0xa,0x20,0x20,0x50,0x68,0x79,\
                        0x73,0x2e,0x20,0x4c,0x65,0x74,0x74,0x2e,0x20,0x52,0x65,0x76,0x2e,0x20,0x34,\
                        0x32,0x2c,0x20,0x33,0x31,0x34,0x31,0x35,0x39,0x20,0x28,0x32,0x30,0x30,0x38,\
                        0x29,0xa,0x20,0x20,0x22,0x45,0x66,0x66,0x65,0x63,0x74,0x20,0x6f,0x66,\
                        0x20,0x4f,0x62,0x73,0x65,0x73,0x73,0x69,0x76,0x65,0x20,0x44,0x65,0x61,0x64,\
                        0x6c,0x69,0x6e,0x65,0x73,0x20,0x69,0x6e,0x20,0x74,0x68,0x65,0x20,0x53,0x70,\
                        0x6f,0x6e,0x74,0x61,0x6e,0x65,0x6f,0x75,0x73,0x20,0x46,0x6f,0x72,0x6d,0x61,\
                        0x74,0x69,0x6f,0x6e,0x20,0x6f,0x66,0x20,0x45,0x61,0x73,0x74,0x65,0x72,0x20,\
                        0x45,0x67,0x67,0x73,0x22,0xa,0x20,0x20,0x4a,0x2e,0x20,0x50,0x65,0x72,\
                        0x61,0x6c,0x74,0x61,0x2c,0x20,0x43,0x2e,0x20,0x4c,0x6f,0x79,0x6f,0x6c,0x61,\
                        0x2c,0x20,0x46,0x2e,0x20,0x47,0x6f,0x6e,0x7a,0x61,0x6c,0x65,0x7a,0x2c,0x20,\
                        0x53,0x2e,0x20,0x44,0x61,0x76,0x69,0x73,0x20,0x28,0x65,0x74,0x20,0x61,0x6c,0x29,0xa,0x0};
  if (!(clp["-magic2"] == "true") && !(clp["-magic3"] == "true")) EndWithError(error1);
  else if ((clp["-magic3"] == "true") && !(clp["-magic2"] == "true")) { ShowWarning(cmdname, warn1); exit(0); }
  else if ((clp["-magic2"] == "true") && !(clp["-magic3"] == "true")) { ShowWarning(cmdname, warn2); exit(0); }
  else if ((clp["-magic2"] == "true") && (clp["-magic3"] == "true")) 
  { 
   std::cerr << mesg1; std::cerr.flush();
   for (int p=0;p<=10;++p) { std::cerr << char(35); sleep(2); }
   std::cerr << mesg2;
   exit(0); 
  }
 }
 if (param.Defined("input-genline")) param.ParseLine(param["input-genline"]);
 if (param.Defined("output-genline")) param.ParseLine(param["output-genline"]);
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
  std::vector<std::string> vv = StringSplit< std::vector<std::string> >(clp["vector-ax,ay,az,bx,by,bz,cx,cy,cz"], ',');
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
  std::vector<std::string> ll = StringSplit< std::vector<std::string> >(clp["lengths-a,b,c"], ',');
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
  std::vector<std::string> aa = StringSplit< std::vector<std::string> >(clp["angles-alpha,beta,gamma"], ',');
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

