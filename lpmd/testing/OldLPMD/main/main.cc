//
//
//

#include "main.h"
#include "cmdline.h"
#include "config.h"

#include <lpmd/util.h>

#include <lpmd/session.h>
#include <lpmd/containable.h>
#include <lpmd/potentialarray.h>
#include <lpmd/simulationcell.h>
#include <lpmd/cellformat.h>
#include <lpmd/cellmanager.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

using namespace lpmd;

Simulator::Simulator(): CommonHandler("lpmd", "LPMD")
{
 GlobalSession.DefineKeyword("showcoords", "true");
 GlobalSession.DefineKeyword("showunused", "true");
 inp = new LPMDInputReader(pluginman, GlobalSession);
 SetInputReader(*inp);
}

Simulator::~Simulator() 
{
 CommonInputReader & param = GetInputReader();
 std::vector<std::string> properties = StringSplit< std::vector<std::string> >(param["property-list"]);
 for (unsigned int i=0;i<propfiles.size();++i) delete propfiles[i];
 std::vector<std::string> atomtypes = StringSplit< std::vector<std::string> >(param["type-list"]);
 for (unsigned int i=0;i<atomtypes.size();++i) delete atomtypemap[atomtypes[i]];
 CommonInputReader * inp = &param;
 delete inp;
}

void Simulator::LoadUseModules()
{
 t.Start();
 std::ostringstream str;
 str << "Las Palmeras Molecular Dynamics, version " << VERSION;
 BannerPrint(str.str());
 std::cout << std::endl;
 LoadAtomTypes();
 CommonHandler::LoadUseModules();
 pluginman.LoadPlugin("energy", "energy-monitor", "");
 pluginman.LoadPlugin("cell", "cell-monitor", "");
}

//
//
//
void Simulator::LoadModules()
{
 CommonHandler::LoadModules();
 CommonInputReader & param = GetInputReader();

 // Asigna los integradores dinamicamente
 if (! param.Defined("integrator-module")) EndWithError("No integrator was defined. Maybe you forgot to put an \"integrator\" statement?");

 BannerPrint("INTEGRATOR INFORMATION");
 std::vector<std::string> integrators = StringSplit< std::vector<std::string> >(param["integrator-list"]);
 for (unsigned int i=0;i<integrators.size();++i)
 {
  Module & pmod = pluginman[integrators[i]];
  Integrator & integ = CastModule<Integrator>(pmod);
  Stepper & ap = CastModule<Stepper>(pmod);
  integlist.push_back(&integ);
  pmod.SetUsed();
  std::cout << "From step:" << ap.start << std::endl;
  pmod.Show(std::cout);
  std::cout << '\n';
 }

 std::vector<std::string> modifiers = StringSplit< std::vector<std::string> >(param["apply-list"]);
 for (unsigned int i=0;i<modifiers.size();++i)
 {
  Module & pmod = pluginman[modifiers[i]];
  SystemModifier & sm = CastModule<SystemModifier>(pmod);
  modiflist.push_back(&sm);
  pmod.SetUsed();
 }

 std::vector<std::string> properties = StringSplit< std::vector<std::string> >(param["property-list"]);
 for (unsigned int i=0;i<properties.size();++i)
 {
  Module & pmod = pluginman[properties[i]];
  InstantProperty & ip = CastModule<InstantProperty>(pmod);
  proplist.push_back(&ip);
  propfiles.push_back(new std::ofstream(pmod["output"].c_str(), std::ios::trunc));
  pmod.SetUsed();
 }

 BannerPrint("SIMULATION MONITOR AND RUNNING AVERAGES INFORMATION");
 std::cout << "Instant monitoring of properties:\n";
 for (std::list<MonitorApplyInfo>::iterator it=inp->monapply.begin();it!=inp->monapply.end();++it)
 {
  MonitorApplyInfo & mon = (*it);
  if (mon.output == "") std::cout << "To standard output: \n";
  else std::cout << "To file " << mon.output << ": \n";
  std::list<std::string> namedprops = mon.properties;
  for (std::list<std::string>::const_iterator it=namedprops.begin();it!=namedprops.end();++it)
  {
   const std::string propname(*it);
   Module & pmod = pluginman.Provider(propname);
   std::cout << "  -> " << propname << " from module \"" << pmod.Name();
   InstantProperty & ip = CastModule<InstantProperty>(pmod);
   if (mon.end == -1) mon.end = param.GetInteger("steps-n");
   ip.start = mon.start;
   ip.end = mon.end;
   ip.each = mon.each;
   std::cout << "\" (start=" << ip.start << ", end=" << ip.end << ", each=" << ip.each << ")\n";
   monitorlist.push_back(&ip);
   pmod.SetUsed();
  }
  std::cout << '\n';
 }
 std::cout << "Running averages of properties:\n";
 for (std::list<RunningAverageApplyInfo>::iterator it=inp->ravapply.begin();it!=inp->ravapply.end();++it)
 {
  RunningAverageApplyInfo & rav = (*it);
  if (rav.output == "") std::cout << "To standard output: \n";
  else std::cout << "To file " << rav.output << ": \n";
  std::list<std::string> namedprops = rav.properties;
  for (std::list<std::string>::const_iterator it=namedprops.begin();it!=namedprops.end();++it)
  {
   const std::string propname(*it);
   Module & pmod = pluginman.Provider(propname);
   std::cout << "  -> " << propname << " from module \"" << pmod.Name();
   InstantProperty & ip = CastModule<InstantProperty>(pmod);
   if (rav.end == -1) rav.end = param.GetInteger("steps-n");
   ip.start = rav.start;
   ip.end = rav.end;
   ip.each = rav.each;
   std::cout << "\" (start=" << ip.start << ", end=" << ip.end << ", each=" << ip.each << ", over=" << rav.average_over << ")\n";
   monitorlist.push_back(&ip);
   pmod.SetUsed();
  }
  std::cout << '\n';
 }
 std::vector<std::string> visualizers = StringSplit< std::vector<std::string> >(param["visualize-list"]);
 for (unsigned int i=0;i<visualizers.size();++i)
 {
  Module & pmod = pluginman[visualizers[i]];
  Visualizer & vis = CastModule<Visualizer>(pmod);
  vislist.push_back(&vis);
  pmod.SetUsed();
 }
}


//
//
//
void Simulator::Initialize()
{
 CommonInputReader & param = GetInputReader();
 if (param["restore-dumpfile"] == "") InitializeFromInput();
 else 
 {
  bool px, py, pz;
  px = param.GetBool("periodic-x");
  py = param.GetBool("periodic-y");
  pz = param.GetBool("periodic-z");

  scell = new SimulationCell(1, 1, 1, px, py, pz);
  scell->GetCell()[0] = e1;
  scell->GetCell()[1] = e2;
  scell->GetCell()[2] = e3;
  SetCell(*scell);

  BannerPrint("SIMULATION RESTORE INFORMATION");
  std::cout << "Restore file ->" << std::endl;
  std::cout << "          file = " << param["restore-dumpfile"] << std::endl;

 // FIXME: MD::LoadDump fue comentada en el api 0.6
 //  LoadDump(param["restore-dumpfile"]);
 }

 SimulationCell & sc = GetCell();
 sc.NumEspec();
 sc.AssignIndex();

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
  for(unsigned long int i=0;i<sc.size();i++)
  {
   if (sc[i].Symb() == spec)
   {
    sc[i].SetCharge(param.GetDouble("charge-"+spec)); 
    N++;
   }
  }
  std::cout << "Set " << N << " atoms of \'" << spec << "\' with a Charge of " << param.GetDouble("charge-"+spec) << '\n'; 
  std::cout << "Mass of " << spec << " atoms  = " << ElemMass[ElemNum(spec)] << '\n';
  totalcharge+=N*param.GetDouble("charge-"+spec);
 }
 std::cout << "Total electric charge of the system is = " << totalcharge << " eV. \n";

 if (param.Defined("cellmanager-module"))
 {
  // Carga el plugin CellManager y lo asigna a la celda de simulacion
  Module & pmod = pluginman[param["cellmanager-module"]];
  CellManager & cm = CastModule<CellManager>(pmod);
  std::cerr << "DEBUG before SetCellManager\n";
  sc.SetCellManager(cm);
  std::cerr << "DEBUG after SetCellManager\n";
  pmod.SetUsed();
  BannerPrint("CELL MANAGER INFORMATION");
  pmod.Show(std::cout);
  std::cout << '\n';
 }

 BannerPrint("CELL INFORMATION");
 std::cout << "Vectors ->                                                              " << std::endl;
 std::cout << "   a = " << sc.GetCell()[0] << std::endl;
 std::cout << "   b = " << sc.GetCell()[1] << std::endl;
 std::cout << "   c = " << sc.GetCell()[2] << std::endl;
 std::cout << "Volume  ->                                                              " << std::endl;
 std::cout << "   V = " << sc.Volume() << " [A^3]." << std::endl;

 // Muestra la informacion de los AtomType a aplicar
 BannerPrint("ATOMIC TYPES INFORMATION");
 if (inp->atapply.size() == 0) std::cout << "No atomic types were selected to be applied\n";
 for (std::list<AtomTypeApplyInfo>::iterator it=inp->atapply.begin();it!=inp->atapply.end();++it)
 {
  AtomTypeApplyInfo & atinf = (*it);
  std::cout << "Type " << atinf.name << " (from atoms " << atinf.from_index << " to " << atinf.to_index << ", starting from step ";
  std::cout << atinf.start << ") : \n";
  if (atomtypemap.count(atinf.name) > 0)
  {
   AtomType * this_type = atomtypemap[atinf.name]; 
   atinf.t = this_type;
   std::cout << (*this_type) << std::endl;
  }
  else EndWithError("AtomType \""+atinf.name+"\" was not defined. Maybe you forgot a \"type\" block?");
 }
 
 if (param.GetBool("showcoords"))
 {
  BannerPrint("ATOMIC COORDINATES INFORMATION");
  std::cout << "Atoms   ->" << std::endl;
  std::cout << "   N = " << sc.size() << std::endl << std::endl;
  std::cout << "Coordinates ->" << std::endl;
  for (unsigned long int i=0;i<sc.size();++i)
  {
   const Atom & a = sc[i];
   std::cout << "   " << a.Symb() << "  " << a.Position();
   if (a.IsTypeSet()) std::cout << " (type " << a.Type().name << ")";
   std::cout << std::endl;
  }
 }
}

void Simulator::InitializeFromInput()
{
 CommonHandler::Initialize();
 CommonInputReader & param = GetInputReader();
 pluginman[param["input-module"]].SetUsed();
 BannerPrint("CELL GENERATOR INFORMATION");
 pluginman[param["input-module"]].Show(std::cout);

 CellGenerator & cg = CastModule<CellGenerator>(pluginman[param["input-module"]]);
 cg.Generate(*scell);
 scell->NumEspec();
 pluginman.UnloadPlugin(param["input-module"]);

 SetCell(*scell);

 // Aplica los modulos especificados en "prepare"
 SimulationCell & sc = GetCell();
 for (std::list<ModuleInfo>::const_iterator it=param.preparelist.begin();it != param.preparelist.end();++it)
 {
  const ModuleInfo & minf = *it;
  pluginman.LoadPlugin(minf.name, minf.id, minf.args);
  pluginman[minf.id].SetUsed();  
  BannerPrint("PREPARE MODULE INFORMATION");
  pluginman[minf.id].Show(std::cout);
  SystemModifier & sm = CastModule<SystemModifier>(pluginman[minf.id]);
  sm.Apply(sc);
  pluginman.UnloadPlugin(minf.id);
 }


}
//
//
//
void Simulator::LoadAtomTypes()
{
 CommonInputReader & param = GetInputReader();
 std::vector<std::string> atomtypes = StringSplit< std::vector<std::string> >(param["type-list"]);
 for (unsigned int i=0;i<atomtypes.size();++i)
 {
  AtomType * this_type = new AtomType(atomtypes[i]);
  std::vector<std::string> t_args = StringSplit< std::vector<std::string> >(param["type-"+atomtypes[i]+"-args"]);
  for (unsigned int j=0;j<t_args.size();++j)
  {
   if (t_args[j] == "fixedpos") this_type->AssignParameter("fixedpos", t_args[(j++)+1]); 
   else if (t_args[j] == "fixedvel") this_type->AssignParameter("fixedvel", t_args[(j++)+1]);
   else if (t_args[j] == "fixedacc") this_type->AssignParameter("fixedacc", t_args[(j++)+1]);
   else
   {
    ShowWarning("lpmd", "Unknown type parameter: "+t_args[j]+", assigned anyway...");
    this_type->AssignParameter(t_args[j], t_args[j+1]);
    j++;
   }
  }
  atomtypemap[atomtypes[i]] = this_type;
 }
}

void Simulator::ShowStartInfo()
{
 CommonInputReader & param = GetInputReader();
 BannerPrint("SYSTEM MODIFIERS INFORMATION");
 for (std::vector<SystemModifier *>::iterator it=modiflist.begin();it!=modiflist.end();++it)
 {
  SystemModifier & sm = *(*it);
  std::cout << "From step " << sm.start << " to step " << sm.end << " each " << sm.each << " steps:" << std::endl;
  Module & pmod = dynamic_cast<Module &>(sm); // no es necesario CastModule aqui
  pmod.Show(std::cout);
  std::cout << '\n';
 }
 BannerPrint("VISUALIZERS INFORMATION");
 for (std::vector<Visualizer *>::iterator it=vislist.begin();it!=vislist.end();++it)
 {
  Visualizer & vis = *(*it);
  std::cout << "From step " << vis.start << " to step " << vis.end << " each " << vis.each << " steps:" << std::endl;
  Module & pmod = dynamic_cast<Module &>(vis); // no es necesario CastModule aqui
  pmod.Show(std::cout);
  std::cout << '\n';
 }
 BannerPrint("PROPERTIES INFORMATION");
 for (std::vector<InstantProperty *>::iterator it=proplist.begin();it!=proplist.end();++it)
 {
  InstantProperty & ip = *(*it);
  std::cout << "From step " << ip.start << " to step " << ip.end << " each " << ip.each << " steps:" << std::endl;
  Module & pmod = dynamic_cast<Module &>(ip); // no es necesario CastModule aqui
  pmod.Show(std::cout);
  std::cout << '\n';
 }
 if (param.GetBool("showunused"))
 {
  BannerPrint("UNUSED MODULES INFORMATION");
  for (std::list<ModuleInfo>::const_iterator it=param.uselist.begin();it != param.uselist.end();++it)
  {
   const ModuleInfo & minf = *it;
   Module & pmod = pluginman[minf.id];
   if (!pmod.Used()) 
   {
    pmod.Show(std::cout);
    std::cout << std::endl;
   }
  }
 }
}

//
//
//
void Simulator::Process()
{
 ShowStartInfo();

 PotentialArray & p_array = GetPotentialArray();
 CommonInputReader & param = GetInputReader();

 long nsteps = param.GetInteger("steps-n");

 BannerPrint("MOLECULAR DYNAMICS INFORMATION");
 std::cout << "Steps   ->" << std::endl;
 std::cout << "           steps = " << nsteps << std::endl;

 SimulationCell & sc = GetCell();
 
 CellWriter ** cf = NULL;
 std::ofstream ** outfile = NULL;
 BannerPrint("WRITING INFORMATION");
 if (param.outputlist.size() > 0)
 {
  cf = new CellWriter*[param.outputlist.size()];
  outfile = new std::ofstream*[param.outputlist.size()];
  // Carga los modulos para escritura 
  int q = 0;
  for (std::list<ModuleInfo>::const_iterator it=param.outputlist.begin();it != param.outputlist.end();++it)
  {
   const ModuleInfo & minf = *it;
   pluginman.LoadPlugin(minf.name, minf.id, minf.args);
   pluginman[minf.id].SetUsed();
   pluginman[minf.id].Show(std::cout);
   std::cout << '\n';
   cf[q] = dynamic_cast<CellWriter *>(&(pluginman[minf.id])); //FIXME: que hacer aqui?
   outfile[q] = new std::ofstream((cf[q]->GetFile()).c_str(), std::ios::out);
   cf[q]->WriteHeader(*(outfile[q]));
   q++;
  }
 }
 else std::cout << "  No output modules were selected." << '\n' << '\n';

 for (std::list<MonitorApplyInfo>::iterator it=inp->monapply.begin();it!=inp->monapply.end();++it)
 {
  MonitorApplyInfo & mon = (*it);
  if (mon.output != "") mon.mout = new std::ofstream(mon.output.c_str(), std::ios::out);
 }
 for (std::list<RunningAverageApplyInfo>::iterator it=inp->ravapply.begin();it!=inp->ravapply.end();++it)
 {
  RunningAverageApplyInfo & rav = (*it);
  if (rav.output != "") rav.mout = new std::ofstream(rav.output.c_str(), std::ios::out);
 } 

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

 for (std::list<MonitorApplyInfo>::iterator it=inp->monapply.begin();it!=inp->monapply.end();++it)
 {
  MonitorApplyInfo & mon = (*it);
  *(mon.mout) << "# step     ";
  for (std::list<std::string>::const_iterator it=mon.properties.begin();it!=mon.properties.end();++it) { *(mon.mout) << std::setw(21) << *it; }
  *(mon.mout) << "\n\n";
 }
 for (std::list<RunningAverageApplyInfo>::iterator it=inp->ravapply.begin();it!=inp->ravapply.end();++it)
 {
  RunningAverageApplyInfo & rav = (*it);
  *(rav.mout) << "# step     ";
  for (std::list<std::string>::const_iterator it=rav.properties.begin();it!=rav.properties.end();++it) { *(rav.mout) << std::setw(21) << *it; }
  *(rav.mout) << "\n\n";
 }

 //
 // MD Loop
 //

 // Assign the values for bond lengths into sc.MetaData()
 for (std::map<std::string, double>::const_iterator it=inp->bondtable.begin();it!=inp->bondtable.end();++it)
 {
  sc.MetaData().AssignParameter((*it).first, ToString<double>((*it).second));
 }

 sc.ClearForces();
 Potential & parray = GetPotentialArray();
 parray.UpdateForces(sc);
 for (step=initial_step;step<nsteps;++step)
 {
  // Checkea la lista de atomtypes para ver cuales deben ser activados
  for (std::list<AtomTypeApplyInfo>::iterator it=inp->atapply.begin();it!=inp->atapply.end();++it)
  {
   AtomTypeApplyInfo & atinf = (*it);
   if ((step+1) >= atinf.start)
   {
    if (Verbose()) std::cerr << "-> Setting AtomType " << atinf.name << " for atoms " << atinf.from_index << " to " << atinf.to_index << '\n';
    for (long q=atinf.from_index;q<=atinf.to_index;++q) sc[q].SetType(*(atinf.t));
    atinf.start = nsteps+1; // FIXME: si alguien piensa en un mejor parche que este...
   }
  }

  // Checkea la lista de integradores para ver cual debe ser activado
  for (std::vector<Integrator *>::iterator it=integlist.begin();it!=integlist.end();++it)
  {
   Integrator & integ = *(*it);
   Module & pmod = dynamic_cast<Module &>(integ); // no es necesario CastModule aqui
   Stepper & ap = CastModule<Stepper>(pmod);
   if ((step+1) >= ap.start)
   { 
    if (Verbose()) std::cerr << "-> Changing active integrator to " << pmod.Name() << '\n';
    SetIntegrator(integ);
    ap.start = nsteps+1; // FIXME: si alguien piensa en un mejor parche que este...
   }
  }

  if (step % param.GetInteger("dumping-each") == 0)
  {
   // FIXME: MD::Dump fue comentada en el api 0.6
   // Dump(param.GetString("dumping-dumpfile")); 
  }

  // Escribe la configuracion con cada modulo llamado en 'output'
  for (int q=0;q<param.outputlist.size();++q)
      if (step % cf[q]->GetInterval() == 0) cf[q]->WriteCell(*(outfile[q]), sc);

  // Check the list of system modifiers to see which we must apply now 
  for (std::vector<SystemModifier *>::iterator it=modiflist.begin();it!=modiflist.end();++it)
  {
   SystemModifier & sm = *(*it);
   if (sm.IsActiveInStep(step)) sm.Apply(*this);
  }

  // Check the list of visualizers to see which we must apply now 
  for (std::vector<Visualizer *>::iterator it=vislist.begin();it!=vislist.end();++it)
  {
   Visualizer & vis = *(*it);
   if (vis.IsActiveInStep(step)) vis.Apply(*this);
  }

  // Check the list of properties to see which we must calculate now  
  long iprop = 0;
  for (std::vector<InstantProperty *>::iterator it=proplist.begin();it!=proplist.end();++it)
  {
   InstantProperty & ip = *(*it);
   Module & pmod = dynamic_cast<Module &>(ip); // no es necesario CastModule aqui
   IContainable & c = CastModule<IContainable>(pmod);
   if(ip.end == nsteps) ip.end--;
   if (ip.IsActiveInStep(step))
   {
    ip.Evaluate(sc, p_array);
    if (pmod.GetBool("average") == false)
    {
     c.OutputTo(*(propfiles[iprop]));
     propfiles[iprop]->flush();
    }
    else c.AddToAverage();
   }
   if (step == ip.end)
   {
    if (pmod.GetBool("average") == true)
    {
     c.OutputAverageTo(*(propfiles[iprop]));
     propfiles[iprop]->flush();
    }
   }
   iprop++;
  }

  for (std::list<MonitorApplyInfo>::iterator it=inp->monapply.begin();it!=inp->monapply.end();++it)
  {
   MonitorApplyInfo & mon = (*it);
   if (mon.IsActiveInStep(step))
   {
    // Actualiza todos los modulos llamados implicitamente por monitor
    for (std::vector<InstantProperty *>::iterator it=monitorlist.begin();it!=monitorlist.end();++it) (*it)->Evaluate(sc, p_array);
    Monitor(mon);
   }
  }
  for (std::list<RunningAverageApplyInfo>::iterator it=inp->ravapply.begin();it!=inp->ravapply.end();++it)
  {
   RunningAverageApplyInfo & rav = (*it);
   if (rav.IsActiveInStep(step))
   {
    // Actualiza todos los modulos llamados implicitamente por monitor
    for (std::vector<InstantProperty *>::iterator it=monitorlist.begin();it!=monitorlist.end();++it) (*it)->Evaluate(sc, p_array);
    RunningAverage(rav);
   }
  }
  sc.MetaData().AssignParameter("step", ToString<int>(step));
  DoStep();
 }

 // Escribe las configuraciones antes de terminar la simulacion completamente.
 for (int q=0;q<param.outputlist.size();++q) outfile[q]->flush();

 if (outfile != NULL) delete [] outfile;
 if (cf != NULL) delete [] cf;
 
 for (std::list<MonitorApplyInfo>::iterator it=inp->monapply.begin();it!=inp->monapply.end();++it)
 {
  MonitorApplyInfo & mon = (*it);
  if (mon.output != "") delete mon.mout;
 }
 for (std::list<RunningAverageApplyInfo>::iterator it=inp->ravapply.begin();it!=inp->ravapply.end();++it)
 {
  RunningAverageApplyInfo & rav = (*it);
  if (rav.output != "") delete rav.mout;
 }
}

void Simulator::Monitor(MonitorApplyInfo & mon)
{
 CommonInputReader & param = GetInputReader();
 *(mon.mout) << std::setw(10) << std::left << step;
 for (std::list<std::string>::const_iterator it=mon.properties.begin();it!=mon.properties.end();++it)
 {
  Module & pmod = pluginman.Provider(*it);
  double pvalue = pmod.GetProperty(*it);
  *(mon.mout) << std::setw(21) << pvalue;
 }
 *(mon.mout) << '\n';
}

void Simulator::RunningAverage(RunningAverageApplyInfo & rav)
{
 CommonInputReader & param = GetInputReader();
 *(rav.mout) << std::setw(10) << std::left << step;
 for (std::list<std::string>::const_iterator it=rav.properties.begin();it!=rav.properties.end();++it)
 {
  Module & pmod = pluginman.Provider(*it);
  double pvalue = pmod.GetProperty(*it);
  rav.AddNewValue(pvalue);
  *(rav.mout) << std::setw(21) << rav.Average();
 }
 *(rav.mout) << '\n';
}

void Simulator::Finish()
{
 BannerPrint("SIMULATION FINISHED!");
 t.Stop();
 t.ShowElapsedTimes();
}

//
// Main Program
//
int main(int argc, char **argv)
{
 srand48(long(time(NULL)));
 Simulator handler;
 try 
 { 
  LPMDCmdLineParser clp(argc, argv);
  ShowLogo();
  handler.Execute(clp); 
 }
 catch (std::exception & ex) { EndWithError(ex.what()); }
 return 0;
}
