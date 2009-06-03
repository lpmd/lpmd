/*
 *
 *
 *
 */

#include "replayintegrator.h"
#include <lpmd/simulation.h>
#include <lpmd/potential.h>

using namespace lpmd;

//
// Por ahora ReplayIntegrator esta vacio
// La idea es que se encargue de tratar la carga de archivos 
// en memoria como si fuera un Integrator (incluso retroceder en configuraciones?)
//

ReplayIntegrator::ReplayIntegrator() { }

void ReplayIntegrator::Advance(Simulation & sim, Potential & pot) { }

