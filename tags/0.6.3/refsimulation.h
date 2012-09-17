/*
 *
 *
 *
 */

#ifndef __LPMD_REFSIMULATION_H__
#define __LPMD_REFSIMULATION_H__

#include <lpmd/simulation.h>
#include <lpmd/integrator.h>
#include <lpmd/combinedpotential.h>
#include <lpmd/basicparticleset.h>
#include <lpmd/particleset.h>
#include <lpmd/basiccell.h>

class RefSimulation: public lpmd::Simulation
{
 public:
    RefSimulation(lpmd::Configuration & conf): step(0), integ(0) 
    { 
     cell = new lpmd::Cell(conf.Cell());
     atoms = new lpmd::ParticleSet(conf.Atoms());
    }

    ~RefSimulation()
    {
     delete cell;
     delete atoms;
    }

    lpmd::BasicCell & OriginalCell() { return (*cell); }

    const lpmd::BasicCell & OriginalCell() const { return (*cell); }

    lpmd::BasicParticleSet & OriginalAtoms() { return (*atoms); }

    const lpmd::BasicParticleSet & OriginalAtoms() const { return (*atoms); }

    void SetTemperature(double temp, bool tag) { }

    lpmd::CombinedPotential & Potentials() { return potarray; }

    const lpmd::CombinedPotential & Potentials() const { return potarray; }

    void DoStep() { step++; }

    void DoSteps(long int n) { step += n; }

    long int CurrentStep() const { return step; }

    void AdjustCurrentStep(long int s) { step = s; }

    void SetIntegrator(lpmd::Integrator & itg) { integ = &itg; }

    lpmd::Integrator & Integrator() { return *integ; }

    void RescalePositions(const lpmd::BasicCell & old_cell) { }

    void Dump(const std::string & path) const { }

    void Restore(const std::string & path) { }

 private:
    long step;
    lpmd::Integrator * integ;
    lpmd::CombinedPotential potarray;
    lpmd::BasicCell * cell;
    lpmd::BasicParticleSet * atoms;
};

#endif

