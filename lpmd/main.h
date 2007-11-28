//
//
//

#include <vector>

#include <lpmd/md.h>
#include <lpmd/pluginmanager.h>
#include <lpmd/instantproperty.h>
#include <lpmd/systemmodifier.h>
#include <lpmd/visualizer.h>

#include "input.h"

class LPMD: public lpmd::MD
{
 public:
  //
  LPMDInputReader param;

  // Constructor y destructor 
  LPMD();
  ~LPMD();

  // From MD
  void Initialize();

  // 
  void LoadSystemFile(std::string sysfile);
  void InitializeFromInput();

  void LoadModules();
  void LoadAtomTypes();
  void ShowStartInfo();
  void RunSimulation();

 private:
   lpmd::SimulationCell *scell;
   lpmd::PluginManager pluginman;
   std::map<std::string, std::ostream *> propfiles;
   std::map<std::string, lpmd::AtomType *> atomtypemap;
   std::vector<lpmd::SystemModifier *> modiflist;
   std::vector<lpmd::InstantProperty *> proplist;
   std::vector<lpmd::Visualizer *> vislist;
   std::vector<lpmd::Integrator *> integlist;
};

void BannerPrint(std::string text);
void ShowHelp();


