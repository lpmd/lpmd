//
//
//

#include <iostream>
#include <sstream>

#include <lpmd/util.h>
#include <lpmd/timer.h>
#include <lpmd/containable.h>
#include <lpmd/instantproperty.h>
#include <lpmd/cellformat.h>

#include "main.h"
#include "cmdline.h"

using namespace lpmd;

void BannerPrint(std::string text)
{
 std::cout << std::setfill('*');
 std::cout << std::endl;
 std::cout << std::setw(80) << "" << std::endl;
 std::cout << "* " << std::setw(78) << std::left << text+" " << std::endl;
 std::cout << std::setw(80) << "" << std::endl;
 std::cout << std::setfill(' ');
}

void ShowHelp()
{
 std::cerr << "Usage: lpmd [--systemfile <file.sys> | -s <file.sys>] [--version | v]" << '\n';
 std::cerr << "       lpmd [--help | -h ]" << '\n';
 exit(1);
}

//
LPMD::LPMD(): param(pluginman) 
{ 
 std::ostringstream str;
 str << "Las Palmeras Molecular Dynamics, version " << VERSION;
 BannerPrint(str.str());
 std::cout << std::endl;
}

LPMD::~LPMD() 
{
 std::vector<std::string> properties = SplitTextLine(param["property-list"]);
 for (unsigned int i=0;i<properties.size();++i) delete propfiles[properties[i]];
 std::vector<std::string> atomtypes = SplitTextLine(param["type-list"]);
 for (unsigned int i=0;i<atomtypes.size();++i) delete atomtypemap[atomtypes[i]];
 delete scell; 
}

void LPMD::LoadModules()
{
 //
 // Load each of the modules called with "use", passing its parameters
 //
 for (std::list<ModuleInfo>::const_iterator it=param.uselist.begin();it != param.uselist.end();++it)
 {
  const ModuleInfo & minf = *it;
  pluginman.LoadPlugin(minf.name, minf.id, minf.args);
 }

 // Asigna el potencial dinamicamente
 BannerPrint("POTENTIAL INFORMATION");
 PotentialArray & p_array = GetPotentialArray();
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
  std::cerr << "Setting potential " << potname << " for " << sp1 << " and " << sp2 << '\n';
  Potential & pot = dynamic_cast<Potential &>(pluginman[potname]);
  std::cout << "Potential assigned to interaction of " << sp1 << " and " << sp2 << ":" << '\n';
  pluginman[potname].SetUsed();
  pluginman[potname].Show();
  std::cout << '\n';
  p_array.Set(sp1, sp2, pot);
 }

 /*
 SimulationCell & c = GetCell();
 std::list<std::string> spc_pairs = c.SpeciesPairs();
 for (std::list<std::string>::const_iterator it = spc_pairs.begin();it != spc_pairs.end();++it)
 {
  const std::string pair = *(it);
  std::vector<std::string> pspl = SplitSpeciesPair(pair);
  if (! param.Defined("potential-"+pair)) 
     EndWithError("Potential for species "+pair+" was not defined. Maybe you forgot to put a \'potential\' statement?");
  Potential & pot = dynamic_cast<Potential &>(pluginman[param["potential-"+pair]]);
  pluginman[param["potential-"+pair]].SetUsed();
  pluginman[param["potential-"+pair]].Show();
  p_array.Set(pspl[0], pspl[1], pot);
 }
 */

 // Asigna los integradores dinamicamente
 if (! param.Defined("integrator-module")) EndWithError("No integrator was defined. Maybe you forgot to put an \'integrator\' statement?");

 BannerPrint("INTEGRATOR INFORMATION");
 std::vector<std::string> integrators = SplitTextLine(param["integrator-list"]);
 for (unsigned int i=0;i<integrators.size();++i)
 {
  Module & pmod = pluginman[integrators[i]];
  Integrator & integ = dynamic_cast<Integrator &>(pmod);
  IApplicable & ap = dynamic_cast<IApplicable &>(integ);
  integlist.push_back(&integ);
  pmod.SetUsed();
  std::cout << "From step:" << ap.start_step << std::endl;
  pmod.Show();
  std::cout << '\n';
 }

 std::vector<std::string> modifiers = SplitTextLine(param["apply-list"]);
 for (unsigned int i=0;i<modifiers.size();++i)
 {
  Module & pmod = pluginman[modifiers[i]];
  SystemModifier & sm = dynamic_cast<SystemModifier &>(pmod);
  modiflist.push_back(&sm);
  pmod.SetUsed();
 }

 std::vector<std::string> properties = SplitTextLine(param["property-list"]);
 for (unsigned int i=0;i<properties.size();++i)
 {
  Module & pmod = pluginman[properties[i]];
  InstantProperty & ip = dynamic_cast<InstantProperty &>(pmod);
  proplist.push_back(&ip);
  propfiles[properties[i]] = new std::ofstream(pmod["output"].c_str(), std::ios::trunc);
  pmod.SetUsed();
 }

 std::vector<std::string> visualizers = SplitTextLine(param["visualize-list"]);
 for (unsigned int i=0;i<visualizers.size();++i)
 {
  Module & pmod = pluginman[visualizers[i]];
  Visualizer & vis = dynamic_cast<Visualizer &>(pmod);
  vislist.push_back(&vis);
  pmod.SetUsed();
 }
}

//
//
//
void LPMD::LoadAtomTypes()
{
 std::vector<std::string> atomtypes = SplitTextLine(param["type-list"]);
 for (unsigned int i=0;i<atomtypes.size();++i)
 {
  AtomType * this_type = new AtomType(atomtypes[i]);
  std::vector<std::string> t_args = SplitTextLine(param["type-"+atomtypes[i]+"-args"]);
  for (unsigned int j=0;j<t_args.size();++j)
  {
   if (t_args[j] == "fixedpos") this_type->AssignParameter("fixedpos", t_args[(j++)+1]); 
   else if (t_args[j] == "fixedvel") this_type->AssignParameter("fixedvel", t_args[(j++)+1]);
   else if (t_args[j] == "fixedacc") this_type->AssignParameter("fixedacc", t_args[(j++)+1]);
   else EndWithError("Unknown type parameter: " + t_args[j]);
  }
  atomtypemap[atomtypes[i]] = this_type;
 }
}

//
//
//
void LPMD::Initialize()
{
 if (param["restore-file"] == "") InitializeFromInput();
 else 
 {
  int nx, ny, nz;
  nx = param.GetInteger("repeat-x");
  ny = param.GetInteger("repeat-y");
  nz = param.GetInteger("repeat-z");
  bool px, py, pz;
  px = param.GetBool("periodic-x");
  py = param.GetBool("periodic-y");
  pz = param.GetBool("periodic-z");

  scell = new SimulationCell(nx, ny, nz, px, py, pz);
  scell->SetVector(0, Vector(1.0, 0.0, 0.0));
  scell->SetVector(1, Vector(0.0, 1.0, 0.0));
  scell->SetVector(2, Vector(0.0, 0.0, 1.0));
  SetCell(*scell);

  BannerPrint("SIMULATION RESTORE INFORMATION");
  std::cout << "Restore file ->" << std::endl;
  std::cout << "          file = " << param["restore-file"] << std::endl;

  LoadDump(param["restore-file"]);
 }

 SimulationCell & c = GetCell();

 BannerPrint("CELL INFORMATION");
 std::cout << "Vectors ->                                                              " << std::endl;
 std::cout << "           a = " << c.GetVector(0) << std::endl;
 std::cout << "           b = " << c.GetVector(1) << std::endl;
 std::cout << "           c = " << c.GetVector(2) << std::endl;
 std::cout << "Volume  ->                                                              " << std::endl;
 std::cout << "           V = " << c.Volume() << " [A^3]." << std::endl;

 // Asigna los AtomType a los atomos
 BannerPrint("ATOMIC TYPES INFORMATION");
 std::vector<std::string> atomtypes = SplitTextLine(param["type-list"]);
 for (unsigned int j=0;j<atomtypes.size();++j)
 {
  AtomType * this_type = atomtypemap[atomtypes[j]];
  std::vector<std::string> atlimits = SplitTextLine(param["atom-"+atomtypes[j]]);
  std::cout << "Type " << atomtypes[j] << " ( from atoms ";

  for (unsigned int k=0;k<atlimits.size();++k)
  {
   ParamList ptmp;
   std::vector<std::string> ltmp = SplitTextLine(atlimits[k], '-');
   ptmp.AssignParameter("from", ltmp[0]);
   ptmp.AssignParameter("to", ltmp[1]);
   std::cout << ptmp.GetInteger("from") << "-" << ptmp.GetInteger("to") << " ";
   for (long i=ptmp.GetInteger("from");i<=ptmp.GetInteger("to");++i) c.SetAtomType(i, *(this_type));
  } 
  std::cout << ") : " << std::endl;
  std::cout << (*this_type) << std::endl;
 }

 c.UseDistanceCache(param.GetBool("distcache-activated"));

 BannerPrint("ATOMIC COORDINATES INFORMATION");
 std::cout << "Atoms   ->" << std::endl;
 std::cout << "           N = " << c.Size() << std::endl << std::endl;
 std::cout << "Coordinates ->" << std::endl;
 for (int i=0;i<c.Size();++i)
 {
  const Atom & a = c.GetAtom(i);
  std::cout << "           " << a.Symb() << "  " << a.Position();
  if (a.IsTypeSet()) std::cout << " (type " << a.Type().name << ")";
  std::cout << std::endl;
 }
}

void LPMD::InitializeFromInput()
{
 Cell *cell = NULL;
 // Construye la celda de simulacion
 if (param.Defined("cell-alpha"))
 {
  double a, b, c, alpha, beta, gamma;
  a = param.GetDouble("cell-a");
  b = param.GetDouble("cell-b");
  c = param.GetDouble("cell-c");
  alpha = param.GetDouble("cell-alpha")*M_PI/180.0e0;
  beta = param.GetDouble("cell-beta")*M_PI/180.0e0;
  gamma = param.GetDouble("cell-gamma")*M_PI/180.0e0;
  cell = new Cell(a, b, c, alpha, beta, gamma);
 }
 else if (param.Defined("cell-ax"))
 {
  Vector a(param.GetDouble("cell-ax"), param.GetDouble("cell-ay"), param.GetDouble("cell-az"));
  Vector b(param.GetDouble("cell-bx"), param.GetDouble("cell-by"), param.GetDouble("cell-bz"));
  Vector c(param.GetDouble("cell-cx"), param.GetDouble("cell-cy"), param.GetDouble("cell-cz"));
  cell = new Cell(a, b, c);
 }

 int nx, ny, nz;
 nx = param.GetInteger("repeat-x");
 ny = param.GetInteger("repeat-y");
 nz = param.GetInteger("repeat-z");
 bool px, py, pz;
 px = param.GetBool("periodic-x");
 py = param.GetBool("periodic-y");
 pz = param.GetBool("periodic-z");
 
 scell = new SimulationCell(nx, ny, nz, px, py, pz);
 scell->SetCell(*cell);
 delete cell;

 //
 // This loads the input (cell generator) module: 
 //
 if (! param.Defined("input-module")) EndWithError("No se definio el generador de celda. Quiza olvido la linea \'input\'?");

 pluginman.LoadPlugin(param["input-module"], param["input-moduleargs"]);
 BannerPrint("CELL GENERATOR INFORMATION");
 pluginman[param["input-module"]].SetUsed();
 pluginman[param["input-module"]].Show();

 CellGenerator & cg = dynamic_cast<CellGenerator &>(pluginman[param["input-module"]]);
 cg.Generate(*scell);
 scell->NumEspec();
 pluginman.UnloadPlugin(param["input-module"]);

 SetCell(*scell);

 SimulationCell & sc = GetCell();
 //Setea los valores para las cargas de las especies atomicas.
 BannerPrint("CHARGES INFORMATION");
 std::list<std::string> spc_pairs = sc.SpeciesList();
 double totalcharge = 0.0e0;
 for (std::list<std::string>::const_iterator it = spc_pairs.begin();it != spc_pairs.end();++it)
 {
  const std::string spec = *(it);
  double chargespec=0.0e0;
  if (! param.Defined("charge-"+spec)) 
  {
   std::cout << "Set charge of " << spec << " with 0.0. " << '\n';
  }
  else
  {
   chargespec = param.GetDouble("charge-"+spec);
   std::cout << "Set charge of " << spec << " with "<<chargespec<< '\n';
  }
  int N=0;
  for(int i=0;i<sc.Size();i++)
  {
   if((sc.GetAtom(i)).Symb() == spec) 
   {
    Atom tmp = sc.GetAtom(i); 
    tmp.SetCharge(param.GetDouble("charge-"+spec)); 
    sc.SetAtom(tmp,i);
    N++;
   }
  }
  std::cout << "Set " << N << " atoms of \'" << spec << "\' with a Charge of " << param.GetDouble("charge-"+spec) << '\n'; 
  std::cout << "Mass of " << spec << " atoms  = " << ElemMass[ElemNum(spec)] << '\n';
  totalcharge+=N*param.GetDouble("charge-"+spec);
 }
 std::cout << "Total electric charge of the system is = " << totalcharge << " eV. \n";
 //Construye supercelda con los valores de nx, ny, nz laidos de "repeat"
 sc.SuperCell();

 if (param.Defined("temperature-t"))
 {
  double init_temp = param.GetDouble("temperature-t");
  sc.InitVelocities();
  sc.SetTemperature(init_temp);
 }
}

//
//
//
void LPMD::RunSimulation()
{
 long nsteps = param.GetInteger("steps-n");

 BannerPrint("MOLECULAR DYNAMICS INFORMATION");
 std::cout << "Steps   ->" << std::endl;
 std::cout << "           steps = " << nsteps << std::endl;

 SimulationCell & sc = GetCell();

 // Carga Modulo para escritura, modulo no se descarga.
 pluginman.LoadPlugin(param["output-module"], param["output-moduleargs"]);
 BannerPrint("WRITING INFORMATION");
 pluginman[param["output-module"]].SetUsed();
 pluginman[param["output-module"]].Show();
 CellWriter & cf = dynamic_cast<CellWriter &>(pluginman[param["output-module"]]);
 std::ofstream outfile((cf.GetFile()).c_str(), std::ios::out);
 cf.WriteHeader(outfile);

 // Load pressure module (this will be connected to the input file soon)
 pluginman.LoadPlugin("pressure", "");
 InstantProperty & pressureProp = dynamic_cast<InstantProperty &>(pluginman["pressure"]);
 IContainable & pressureCont = dynamic_cast<IContainable &>(pluginman["pressure"]);
 ModuleInfo tminf("pressure", "pressure", "");
 param.uselist.push_back(tminf);

 double K, U, E, T, V;

 PotentialArray & p_array = GetPotentialArray();

 int initial_step;
 if (param["restore-dumpfile"] == "") 
 {
  initial_step = 0; 
  BannerPrint("SIMULATION STARTED!");
 }
 else 
 {
  initial_step = step;
  BannerPrint("SIMULATION CONTINUED!");
 }
 std::cout << "   Steps      K (eV)                U (eV)               E (eV)              T (K)               P (MPa)            V(A^3)\n\n";

 //
 // MD Loop
 //
 for (step=initial_step;step<nsteps;++step)
 {
  // Checkea la lista de integradores para ver cual debe ser activado
  for (std::vector<Integrator *>::iterator it=integlist.begin();it!=integlist.end();++it)
  {
   Integrator & integ = *(*it);
   IApplicable & ap = dynamic_cast<IApplicable &>(integ);
   if ((step+1) == ap.start_step)
   { 
    Module & pmod = dynamic_cast<Module &>(integ);
    std::cerr << "-> Changing active integrator to " << pmod.Name() << '\n';
    SetIntegrator(integ);
   }
  }

  if (step % param.GetInteger("dumping-each") == 0) Dump(param.GetString("dumping-dumpfile")); 

  // Approach de como seria la escritura utilizando la parte modular.
  if (step % cf.GetInterval() == 0) cf.WriteCell(outfile, sc);


  // Check the list of system modifiers to see which we must apply now 
  for (std::vector<SystemModifier *>::iterator it=modiflist.begin();it!=modiflist.end();++it)
  {
   SystemModifier & sm = *(*it);
   if (MustDo(step, sm.start_step, sm.end_step, sm.interval)) sm.Apply(*this);
  }

  // Check the list of visualizers to see which we must apply now 
  for (std::vector<Visualizer *>::iterator it=vislist.begin();it!=vislist.end();++it)
  {
   Visualizer & vis = *(*it);
   if (MustDo(step, vis.start_step, vis.end_step, vis.interval)) vis.Apply(*this);
  }

  // Check the list of properties to see which we must calculate now  
  for (std::vector<InstantProperty *>::iterator it=proplist.begin();it!=proplist.end();++it)
  {
   InstantProperty & ip = *(*it);
   Module & pmod = dynamic_cast<Module &>(ip);
   IContainable & c = dynamic_cast<IContainable &>(pmod);
   if (MustDo(step, ip.start_step, ip.end_step, ip.interval)) 
   {
    ip.Evaluate(sc, p_array);
	if (pmod.GetBool("average") == false)
	{
	 c.OutputTo(*(propfiles[pmod.Name()]));
     propfiles[pmod.Name()]->flush();
	}
	else c.AddToAverage();
   }
   if (step == ip.end_step)
   {
    if (pmod.GetBool("average") == true)
    {
     c.OutputAverageTo(*(propfiles[pmod.Name()]));
     propfiles[pmod.Name()]->flush();
    }
   }
  }

  if (step % param.GetInteger("monitor-each") == 0)
  {
   K = sc.KineticEnergy();
   U = p_array.energy(sc);
   E = K + U;
   T = sc.Temperature();
   V = sc.Volume();

   // Evaluate Pressure: soon all quantities will be evaluated like this
   pressureProp.Evaluate(sc, p_array);

   std::cout << std::setw(10) << std::left << step << std::setw(21) << K << std::setw(21) << U;
   std::cout << std::setw(21) << E << std::setw(21) << T << std::setw(21);
   pressureCont.OutputTo(std::cout);
   std::cout << std::setw(21) << V << '\n';
  }
  
  DoStep();

 }

 BannerPrint("SIMULATION FINISHED!");
}

//
// Show Start Information 
//
void LPMD::ShowStartInfo()
{
 BannerPrint("SYSTEM MODIFIERS INFORMATION");
 for (std::vector<SystemModifier *>::iterator it=modiflist.begin();it!=modiflist.end();++it)
 {
  SystemModifier & sm = *(*it);
  std::cout << "From step " << sm.start_step << " to step " << sm.end_step << " each " << sm.interval << " steps:" << std::endl;
  Module & pmod = dynamic_cast<Module &>(sm);
  pmod.Show();
  std::cout << '\n';
 }

 BannerPrint("VISUALIZERS INFORMATION");
 for (std::vector<Visualizer *>::iterator it=vislist.begin();it!=vislist.end();++it)
 {
  Visualizer & vis = *(*it);
  std::cout << "From step " << vis.start_step << " to step " << vis.end_step << " each " << vis.interval << " steps:" << std::endl;
  Module & pmod = dynamic_cast<Module &>(vis);
  pmod.Show();
  std::cout << '\n';
 }

 BannerPrint("PROPERTIES INFORMATION");
 for (std::vector<InstantProperty *>::iterator it=proplist.begin();it!=proplist.end();++it)
 {
  InstantProperty & ip = *(*it);
  std::cout << "From step " << ip.start_step << " to step " << ip.end_step << " each " << ip.interval << " steps:" << std::endl;
  Module & pmod = dynamic_cast<Module &>(ip);
  pmod.Show();
  std::cout << '\n';
 }

 BannerPrint("UNUSED MODULES INFORMATION");
 for (std::list<ModuleInfo>::const_iterator it=param.uselist.begin();it != param.uselist.end();++it)
 {
  const ModuleInfo & minf = *it;
  Module & pmod = pluginman[minf.id];
  if (!pmod.Used()) 
  {
   pmod.Show();
   std::cout << std::endl;
  }
 }
}

//
// Main Program
//
int main(int argc, char **argv)
{
 srand48(long(time(NULL)));
 Timer t;
 t.Start();

 try
 {
  //
  LPMDCmdLineParser clp(argc, argv);
  if (clp.Defined("help")) ShowHelp();

  LPMD md;
  md.param.Read(clp["systemfile-file"]);
  std::cerr << md.param << '\n';
  std::cout << std::endl;
  int status=md.param.CheckConsistency();
  if (status!=0) { EndWithError("Input file \" " + clp["systemfile-file"] + "\" returned an error"); }

  md.LoadAtomTypes();
  md.Initialize();
  md.LoadModules();
  md.ShowStartInfo();
  md.RunSimulation();
 }
 catch (std::exception & ex) { EndWithError(ex.what()); }

 t.Stop();
 t.ShowElapsedTimes();

 return 0;
}

