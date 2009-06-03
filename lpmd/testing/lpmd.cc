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
 }

 timer.Stop();
 std::cout << "Simulation over " << nsteps << " steps\n";
 timer.ShowElapsedTimes();
}

void LPMD::OpenPropertyStreams()
{
 Array<std::string> properties = StringSplit(control["property-modules"]); 
 for (int p=0;p<properties.Size();++p)
 {
  const Parameter & pluginname = properties[p];
  const std::string filename = pluginmanager[pluginname]["output"];
  propertystream.Append(new std::ofstream(filename.c_str()));
  lpmd::InstantProperty & prop = dynamic_cast<lpmd::InstantProperty &>(pluginmanager[properties[p]]);
  lpmd::AbstractValue & value = dynamic_cast<lpmd::AbstractValue &>(prop);
  value.ClearAverage();
 }
}

void LPMD::ClosePropertyStreams()
{
 Array<std::string> properties = StringSplit(control["property-modules"]); 
 for (int p=0;p<properties.Size();++p)
 {
  lpmd::InstantProperty & prop = dynamic_cast<lpmd::InstantProperty &>(pluginmanager[properties[p]]);
  lpmd::AbstractValue & value = dynamic_cast<lpmd::AbstractValue &>(prop);
  value.OutputAverageTo(*(propertystream[p]));
  delete propertystream[p];
 }
}

void LPMD::ComputeProperties()
{
 long currentstep = simulation->CurrentStep();
 Array<std::string> properties = StringSplit(control["property-modules"]); 
 for (int p=0;p<properties.Size();++p)
 {
  lpmd::InstantProperty & prop = dynamic_cast<lpmd::InstantProperty &>(pluginmanager[properties[p]]);
  if (prop.IsActiveInStep(currentstep)) 
  {
   prop.Evaluate(*simulation, simulation->Potentials());
   lpmd::AbstractValue & value = dynamic_cast<lpmd::AbstractValue &>(prop);
   if (bool(pluginmanager[properties[p]]["average"])) value.AddToAverage();
   else value.OutputTo(*(propertystream[p]));
  }
 }
}

LPMD::LPMD(int argc, const char * argv[]): Application("LPMD", "lpmd", control), control(pluginmanager)
{
 PrintPalmTree();
 ProcessControl(argc, argv);
}

