/*
 *
 *
 *
 */

#ifndef __LPMDUTIL_ANALYZER_H__
#define __LPMDUTIL_ANALYZER_H__

#include "application.h"
#include "replayintegrator.h"
#include <lpmd/property.h>

#include <iostream>

class Analyzer: public Application
{
 public:
   Analyzer(int argc, const char * argv[]);
   ~Analyzer();

   int Run();
   void Iterate();
   void PreReadConfigurations();
   void CheckForTemporalProperties();
   void ComputeTemporalProperties();

 private:
   UtilityControl control; 
   Array<TemporalProperty *> temporalproperties;
   ReplayIntegrator * replay;
};

#endif

