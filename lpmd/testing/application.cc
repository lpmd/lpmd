/*
 *
 *
 *
 */

#include "application.h"
#include <lpmd/simulationbuilder.h>
#include <lpmd/cellgenerator.h>
#include <lpmd/systemmodifier.h>
#include <lpmd/combinedpotential.h>
#include <lpmd/properties.h>

Application::Application(const std::string & appname, UtilityControl & uc): innercontrol(uc)
{
 std::cout << "Este es " << appname << " 0.6.0 alfa\n"; 
}

Application::~Application() { }

int Application::Run()
{
 CheckConsistency();
 // 
 ConstructCell();
 FillAtoms();
 SetPotentials();
 ApplyPrepares();
 //
 Iterate();
 return 0;
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


