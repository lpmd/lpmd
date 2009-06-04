/*
 *
 *
 *
 */

#ifndef __LPMDUTIL_VISUALIZER_H__
#define __LPMDUTIL_VISUALIZER_H__

#include "application.h"
#include <iostream>

class VisualizerControl: public UtilityControl
{
 public:
   VisualizerControl(PluginManager & pm): UtilityControl(pm) { }
};

class Visualizer: public Application
{
 public:
   Visualizer(int argc, const char * argv[]);
   ~Visualizer();

   int Run();
   void FillAtoms();
   void Iterate();

 private:
   VisualizerControl control; 
};

#endif

