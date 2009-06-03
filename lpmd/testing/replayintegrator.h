/*
 *
 *
 *
 */

#ifndef __LPMD_REPLAYINTEGRATOR_H__
#define __LPMD_REPLAYINTEGRATOR_H__

#include <lpmd/integrator.h>

class ReplayIntegrator: public lpmd::Integrator
{
 public:
   ReplayIntegrator();
   void Advance(lpmd::Simulation & sim, lpmd::Potential & pot);

};

#endif

