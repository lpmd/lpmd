//
//
//

#include "../common/handler.h"
#include "input.h"

#include <map>
#include <vector>

#include <lpmd/atom.h>
#include <lpmd/timer.h>
#include <lpmd/instantproperty.h>
#include <lpmd/visualizer.h>
#include <lpmd/integrator.h>
#include <lpmd/systemmodifier.h>

class Simulator: public CommonHandler
{
 public:
  // Constructor y destructor 
  Simulator();
  ~Simulator();

  void Initialize();
  void InitializeFromInput();
  void LoadAtomTypes();
  void ShowStartInfo();
  void Monitor(std::ostream * mout);
  void LoadUseModules();
  void LoadModules();
  void Process();
  void Finish();
  
 private: 
  lpmd::Timer t;
  LPMDInputReader * inp;
  std::map<std::string, std::ostream *> propfiles;
  std::map<std::string, lpmd::AtomType *> atomtypemap;
  std::vector<lpmd::SystemModifier *> modiflist;
  std::vector<lpmd::InstantProperty *> proplist;
  std::vector<lpmd::InstantProperty *> monitorlist;
  std::vector<lpmd::Visualizer *> vislist;
  std::vector<lpmd::Integrator *> integlist;
};

