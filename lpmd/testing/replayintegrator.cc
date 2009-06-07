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
 try { cellreader = dynamic_cast<CellReader *>(&inpplugin); }
 catch (std::exception & e) { cellreader = 0; }
}

void ReplayIntegrator::Advance(Simulation & sim, Potential & pot) 
{ 
 if (cellreader != 0)
 {
  sim.Atoms().Clear();
  bool status = cellreader->ReadCell(inputstream, sim);
  if (! status) throw RuntimeError("No more configurations to read");
 }
}

void ReplayIntegrator::PreRead(Simulation & simulation)
{
 assert(cellreader != 0);
 history.Append(simulation);
 cellreader->ReadMany(inputstream, history, true);
 GlobalSession.DebugStream() << "-> Read " << history.Size() << " configurations.\n";
}

SimulationHistory & ReplayIntegrator::History() { return history; }

