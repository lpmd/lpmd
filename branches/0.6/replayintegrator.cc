/*
 *
 *
 *
 */

#include "replayintegrator.h"
#include <exception>

#include <lpmd/simulation.h>
#include <lpmd/potential.h>
#include <lpmd/session.h>

using namespace lpmd;

ReplayIntegrator::ReplayIntegrator(Plugin & inpplugin, std::istream & inpstream): inputstream(inpstream)
{
 nconf = 1;
 if (!inpplugin.Defined("start")) inpplugin["start"] = "0";
 if (!inpplugin.Defined("end")) inpplugin["end"] = "-1";
 if (!inpplugin.Defined("each")) inpplugin["each"] = "1";
 mystepper.start = int(inpplugin["start"]);
 mystepper.end = int(inpplugin["end"]);
 mystepper.each = int(inpplugin["each"]);
 try { cellreader = dynamic_cast<CellReader *>(&inpplugin); }
 catch (std::exception & e) { cellreader = 0; }
}

void ReplayIntegrator::Advance(Simulation & sim, Potential & pot) 
{ 
 assert(&pot != 0);//icc 869
 if (cellreader != 0)
 {
  BasicParticleSet & atoms = sim.Atoms();
  for (long int i=0;i<atoms.Size();++i) atoms.RemoveTags(atoms[i]);
  atoms.Clear();
  bool status;
  if (mystepper.IsActiveInStep(nconf))
  {
   status = cellreader->ReadCell(inputstream, sim);
   GlobalSession.DebugStream() << "-> Read configuration " << nconf;
  }
  else 
  {
   try
   {
    status = cellreader->SkipCell(inputstream);
    sim.Cell()[0] = Vector(0.0, 0.0, 0.0); // This flags the config as invalid!
    GlobalSession.DebugStream() << "-> Skipped configuration " << nconf << "\n";
   }
   catch (NotImplemented & e)
   {
    status = cellreader->ReadCell(inputstream, sim);
    GlobalSession.DebugStream() << "-> Read (could not skip) configuration " << nconf << "\n";
   }
   nconf++; 
   if ((! status) || ((mystepper.end != -1) && (nconf > mystepper.end)))
      throw RuntimeError("No more configurations to read");
   throw InvalidRequest("skip");
  }
  if ((! status) || ((mystepper.end != -1) && (nconf > mystepper.end)))
     throw RuntimeError("No more configurations to read");
  nconf++;
 }
}

void ReplayIntegrator::PreRead(Simulation & simulation)
{
 assert(cellreader != 0);
 history.Append(simulation);
 cellreader->ReadMany(inputstream, history, mystepper, true);
 GlobalSession.DebugStream() << "-> Read " << history.Size() << " configurations.\n";
}

SimulationHistory & ReplayIntegrator::History() { return history; }

