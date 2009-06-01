/*
 *
 *
 *
 */

#include "lpmd.h"
#include "quickmode.h"
#include <lpmd/combinedpotential.h>
#include <lpmd/integrator.h>
#include <lpmd/properties.h>
#include <lpmd/timer.h>
#include <lpmd/configuration.h>
#include <lpmd/visualizer.h>

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

void LPMD::Iterate()
{
 BasicParticleSet & atoms = simulation->Atoms();
 //
 // Set solver first
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
  
  RunModifiers(i);
  RunVisualizers(i);

  if (i % 100 == 0)
  {
   double kin_en = KineticEnergy(atoms), pot_en = 0.0;
   pot_en += simulation->Potentials().energy(*simulation);
   double tot_en = kin_en + pot_en;
   double temp = Temperature(atoms);
   std::cout << i << "  " << pot_en << "  " << kin_en << "  " << tot_en << "  " << temp << '\n';
  }
 }
 timer.Stop();
 std::cout << "Simulation over " << nsteps << " steps\n";
 timer.ShowElapsedTimes();
}

void LPMD::RunVisualizers(long currentstep)
{
 Array<std::string> visualizers = StringSplit(control["visualize-modules"]);
 for (int v=0;v<visualizers.Size();++v)
 {
  Visualizer & vis = CastModule<Visualizer>(pluginmanager[visualizers[v]]);
  if (vis.IsActiveInStep(currentstep)) vis.Apply(*simulation);
 }
}

void LPMD::RunModifiers(long currentstep)
{
 Array<std::string> modifiers = StringSplit(control["apply-modules"]);
 for (int v=0;v<modifiers.Size();++v)
 {
  SystemModifier & sm = CastModule<SystemModifier>(pluginmanager[modifiers[v]]);
  if (sm.IsActiveInStep(currentstep)) sm.Apply(*simulation);
 }
}

LPMD::LPMD(int argc, const char * argv[]): Application("LPMD", control), control(pluginmanager)
{
 QuickModeParser quick;
 quick.Parse(argc, argv);
 ParamList options;
 if (quick.Arguments().Size() == 1)
 {
  std::istringstream generatedcontrol(quick.FormattedAsControlFile());
  control.Read(generatedcontrol, options, "quickmode"); 
 }
 else control.Read(quick.Arguments()[1], options);  
}

