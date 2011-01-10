/*
 *
 *
 *
 */

#include "lpmd.h"
#include "config.h" 
#include "palmtree.h"
#include <lpmd/combinedpotential.h>
#include <lpmd/integrator.h>
#include <lpmd/properties.h>
#include <lpmd/timer.h>
#include <lpmd/configuration.h>
#include <lpmd/property.h>
#include <lpmd/storedvalue.h>
#include <lpmd/value.h>
#include <lpmd/session.h>

#include <fstream>

int main(int argc, const char * argv[])
{
 try
 {
  LPMD main(argc, argv);
  if (main.innercontrol["filter-end"] == "false") main.ApplyFilters();
  else 
  {
   main.RunModifiers();
   main.ApplyFilters();
  }
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
 if (control.Defined("restore-file")) RestoreSimulation();
 else Application::FillAtoms();
 PrintBanner("CELL MANAGER");
 pluginmanager[control["cellmanager-module"]].Show(std::cout);
 PrintBanner("INITIAL CONFIGURATION");
 simulation->ShowInfo(std::cout);
}

void LPMD::RestoreSimulation()
{
 if (name == "LPMD") 
 {
  PrintBanner("RESTORE INFORMATION"); 
  std::cout << "Restoring from dump file: " << control["restore-file"] << '\n';
  simulation->Restore(control["restore-file"]);
 }
 if (bool(innercontrol["optimize-simulation"])) OptimizeSimulationAtStart();
 else GlobalSession.DebugStream() << "-> NOT optimizing simulation, by user request\n";
 simulation->Cell().Periodicity(0) = bool(innercontrol["periodic-x"]); 
 simulation->Cell().Periodicity(1) = bool(innercontrol["periodic-y"]); 
 simulation->Cell().Periodicity(2) = bool(innercontrol["periodic-z"]); 
}

void LPMD::Iterate()
{
 PrintBanner("SYSTEM MODIFIERS");
 ShowApplicableModules("apply");
 PrintBanner("VISUALIZERS");
 ShowApplicableModules("visualize");
 PrintBanner("INTEGRATOR");
 pluginmanager[control["integrator-module"]].Show(std::cout);

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
 //
 // Initialize dumper Stepper
 //
 if (control.Defined("dumping-each")) 
    dumper = Stepper(0, nsteps, int(control["dumping-each"]));
 // 
 //
 PrintBanner("SIMULATION STARTED"); 
 for (long i=simulation->CurrentStep();i<nsteps;++i)
 {
  UpdateAtomicIndices();
  for (int k=0;k<atoms.Size();++k) atoms[k].Acceleration() = Vector(0.0, 0.0, 0.0);
  simulation->DoStep();
  RunModifiers();
  ComputeProperties();
  RunVisualizers();
  if (control.Defined("extraverbose")) simulation->ShowInfo(std::cout);
  if (control.Defined("dumping-each") && (dumper.IsActiveInStep(simulation->CurrentStep()))) 
     simulation->Dump(control["dumping-file"]);
  SaveCurrentConfiguration();
 }

 simulation->Dump(control["lastconfig"]);
 timer.Stop();
 PrintBanner("SIMULATION FINISHED"); 
 timer.ShowElapsedTimes();
}

LPMD::LPMD(int argc, const char * argv[]): Application("LPMD", "lpmd", control), control(pluginmanager)
{
 PrintPalmTree();
 PrintBanner("LPMD VERSION "+std::string(VERSION));
 ProcessControl(argc, argv, "visualize");
}

