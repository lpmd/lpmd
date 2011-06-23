/*
 *
 *
 *
 */

#ifndef __LPMD_REPLAYINTEGRATOR_H__
#define __LPMD_REPLAYINTEGRATOR_H__

#include <lpmd/plugin.h>
#include <lpmd/cellreader.h>
#include <lpmd/integrator.h>
#include <lpmd/simulation.h>
#include <lpmd/simulationhistory.h>

class ReplayIntegrator: public lpmd::Integrator
{
 public:
   ReplayIntegrator(lpmd::Plugin & inputplugin, std::istream & inputstream);
   void Advance(lpmd::Simulation & sim, lpmd::Potential & pot);
   void PreRead(lpmd::Simulation & simulation);
   lpmd::SimulationHistory & History();

 private:
   long nconf;
   Stepper mystepper;
   std::istream & inputstream;
   lpmd::CellReader * cellreader;
   lpmd::SimulationHistory history;
};

#endif

