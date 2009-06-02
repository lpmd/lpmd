/*
 *
 *
 *
 */

#include "application.h"
#include "config.h"
#include <lpmd/simulationbuilder.h>
#include <lpmd/cellgenerator.h>
#include <lpmd/systemmodifier.h>
#include <lpmd/combinedpotential.h>
#include <lpmd/properties.h>
#include <lpmd/session.h>

#include <iostream>
#include <iomanip>

Application::Application(const std::string & appname, const std::string & cmd, UtilityControl & uc): name(appname), cmdname(cmd), innercontrol(uc)
{

}

Application::~Application() { }

int Application::Run()
{
 CheckConsistency();
 // 
 ConstructCell();
 FillAtoms();
 AdjustAtomProperties();
 SetPotentials();
 ApplyPrepares();
 //
 Iterate();
 return 0;
}

void Application::PrintBanner(const std::string & text)
{
 std::cout << std::setfill('*');
 std::cout << std::endl;
 std::cout << std::setw(80) << "" << std::endl;
 std::cout << "* " << std::setw(78) << std::left << text+" " << std::endl;
 std::cout << std::setw(80) << "" << std::endl;
 std::cout << std::setfill(' ');
}

void Application::ShowHelp()
{
 std::cerr << name << " version " << VERSION;
 std::cerr << '\n';
 std::cerr << "Using liblpmd version " << lpmd::GlobalSession["libraryversion"] << std::endl << std::endl;
 std::cerr << "Usage: " << cmdname << " [--verbose | -v ] [--lengths | -L <a,b,c>] [--angles | -A <alpha,beta,gamma>]";
 std::cerr << " [--vector | -V <ax,ay,az,bx,by,bz,cx,cy,cz>] [--scale | -S <value>]";
 std::cerr << " [--option | -O <option=value,option=value,...>] [--input | -i plugin:opt1,opt2,...] [--output | -o plugin:opt1,opt2,...]";
 std::cerr << " [--use | -u plugin:opt1,opt2,...] [--cellmanager | -c plugin:opt1,opt2,...] [--replace-cell | -r] [file.control]\n";
 std::cerr << "       " << cmdname << " [--pluginhelp | -p <pluginname>]\n";
 std::cerr << "       " << cmdname << " [--help | -h]\n";
 exit(1);
}

void Application::ShowPluginHelp()
{
 std::cout << "Loaded from file: " << pluginmanager["help_plugin"]["fullpath"] << '\n';
 if (pluginmanager["help_plugin"].Defined("version")) 
    std::cout << "Plugin version: " << pluginmanager["help_plugin"]["version"] << '\n';
 std::cout << '\n';
 pluginmanager["help_plugin"].ShowHelp();
 PrintBanner("Provides");
 std::cout << "     >> " << pluginmanager["help_plugin"].Provides() << '\n';
 PrintBanner("Arguments Required or supported");
 std::cout << "     >> " << pluginmanager["help_plugin"].Keywords() << '\n';
 PrintBanner("Default values for parameters");
 pluginmanager["help_plugin"].Show(std::cout);
 exit(1);
}

void Application::CheckConsistency()
{
 std::cerr << '\n' << innercontrol << '\n';
}

void Application::ConstructCell()
{
 std::string celltype = innercontrol["cell-type"];
 if (celltype == "cubic")
 {
  double length = double(innercontrol["cell-a"]);
  double scale = double(innercontrol["cell-scale"]);
  cell[0] = length*scale*e1;
  cell[1] = length*scale*e2;
  cell[2] = length*scale*e3;
 }
 else if (celltype == "crystal")
 {
  double box[3];
  box[0] = double(innercontrol["cell-a"]);
  box[1] = double(innercontrol["cell-b"]);
  box[2] = double(innercontrol["cell-c"]);
  double scale = double(innercontrol["cell-scale"]);
  for (int q=0;q<3;++q) cell[q] = box[q]*scale*identity[q];
 }
 else 
 {
  double ax, ay, az;
  ax = double(innercontrol["cell-ax"]);
  ay = double(innercontrol["cell-ay"]);
  az = double(innercontrol["cell-az"]);
  cell[0] = Vector(ax, ay, az);
  double bx, by, bz;
  bx = double(innercontrol["cell-bx"]);
  by = double(innercontrol["cell-by"]);
  bz = double(innercontrol["cell-bz"]);
  cell[1] = Vector(bx, by, bz);
  double cx, cy, cz;
  cx = double(innercontrol["cell-cx"]);
  cy = double(innercontrol["cell-cy"]);
  cz = double(innercontrol["cell-cz"]);
  cell[2] = Vector(cx, cy, cz);
 }
}

void Application::FillAtoms()
{
 simulation = &(SimulationBuilder::CreateGeneric());
 for (int q=0;q<3;++q) simulation->Cell()[q] = cell[q];
 CellGenerator & cg = CastModule<CellGenerator>(pluginmanager["input1"]);
 pluginmanager["input1"].Show(std::cout);
 cg.Generate(*simulation);
 if (innercontrol.Defined("cellmanager-module"))
 {
  pluginmanager[innercontrol["cellmanager-module"]].Show(std::cout);
  simulation->SetCellManager(CastModule<CellManager>(pluginmanager[innercontrol["cellmanager-module"]]));
 }
}

void Application::AdjustAtomProperties()
{
 BasicParticleSet & atoms = simulation->Atoms();
 // FIXME: por ahora solo se considera atom-group y charge-group 
 // como las especies atomicas 
 for (long int i=0;i<atoms.Size();++i)
 {
  for (int q=0;q<innercontrol.massgroups.Parameters().Size();++q)
     if (atoms[i].Symbol() == innercontrol.massgroups.Parameters()[q]) 
        atoms[i].Mass() = innercontrol.massgroups[innercontrol.massgroups.Parameters()[q]];
  for (int q=0;q<innercontrol.chargegroups.Parameters().Size();++q)
     if (atoms[i].Symbol() == innercontrol.chargegroups.Parameters()[q]) 
        atoms[i].Charge() = innercontrol.chargegroups[innercontrol.chargegroups.Parameters()[q]];
 }
 if (innercontrol.Defined("showinitialconfig"))
 {
  for (long int i=0;i<atoms.Size();++i)
  {
   std::cout << i << ": " << atoms[i].Symbol() << " Mass = " << atoms[i].Mass() << " Charge = " << atoms[i].Charge() << " ";
   std::cout << "Position = " << atoms[i].Position() << '\n';
  }
 }
}

void Application::ApplyPrepares()
{
 Array<std::string> prepares = StringSplit(innercontrol["prepare-modules"]);
 for (int p=0;p<prepares.Size();++p)
 {
  std::string id = "prepare"+ToString(p+1);
  pluginmanager[id].Show(std::cout);
  SystemModifier & sm = CastModule<SystemModifier>(pluginmanager[id]);
  sm.Apply(*simulation);
 } 
}

void Application::SetPotentials()
{
 CombinedPotential & potentials = simulation->Potentials();
 Array<Parameter> pkeys = innercontrol.Potentials().Parameters();
 for (int p=0;p<pkeys.Size();++p) 
 {
  Potential & pot = CastModule<Potential>(pluginmanager[innercontrol.Potentials()[pkeys[p]]]);
  int spc1 = ElemNum(SplitSpeciesPair(pkeys[p])[0]);
  int spc2 = ElemNum(SplitSpeciesPair(pkeys[p])[0]);
  pot.SetValidSpecies(spc1, spc2);
  potentials.Append(pot);
 } 
}

void Application::Iterate()
{


}


