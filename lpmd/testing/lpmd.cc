/*
 *
 *
 *
 */

#include "lpmd.h"
#include "palmtree.h"
#include <lpmd/combinedpotential.h>
#include <lpmd/integrator.h>
#include <lpmd/properties.h>
#include <lpmd/timer.h>
#include <lpmd/configuration.h>
#include <lpmd/property.h>
#include <lpmd/storedvalue.h>
#include <lpmd/value.h>

#include <fstream>

int main(int argc, const char * argv[])
{
 try
 {
  LPMD main(argc, argv);
  return main.Run();
 }
 catch (std::exception & e)
 {
  std::cerr << "[Error] " << e.what() << '\n';
  return 1;
 }
}

void LPMD::FillAtoms() 
{
 Application::FillAtoms();
 simulation->ShowInfo(std::cout);
}

void LPMD::Iterate()
{
 BasicParticleSet & atoms = simulation->Atoms();
 //
 simulation->SetIntegrator(CastModule<Integrator>(pluginmanager[control["integrator-module"]]));
 //
 for (int i=0;i<atoms.Size();++i) atoms[i].Acceleration() = Vector(0.0, 0.0, 0.0);
 simulation->Potentials().Initialize(*simulation);
 simulation->Potentials().UpdateForces(*simulation);

 //
 // MD loop
 //
 Timer timer;
 timer.Start();
 long nsteps = int(control["steps-number"]);
 for (long i=0;i<nsteps;++i)
 {
  for (int k=0;k<atoms.Size();++k) atoms[k].Acceleration() = Vector(0.0, 0.0, 0.0);
  simulation->DoStep();
  
  RunModifiers();
  ComputeProperties();
  RunVisualizers();
  SaveCurrentConfiguration();
 }

 timer.Stop();
 std::cout << "Simulation over " << nsteps << " steps\n";
 timer.ShowElapsedTimes();
}

LPMD::LPMD(int argc, const char * argv[]): Application("LPMD", "lpmd", control), control(pluginmanager)
{
 PrintPalmTree();
 ProcessControl(argc, argv, "visualize");
}

