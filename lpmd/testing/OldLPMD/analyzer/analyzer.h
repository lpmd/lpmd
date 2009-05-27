//
//
//

#include "../common/handler.h"
#include "input.h"

#include <lpmd/instantproperty.h>
#include <lpmd/temporalproperty.h>

#include <vector>

class Analyzer: public CommonHandler
{
 public:
  // Constructor y destructor 
  Analyzer();
  ~Analyzer();

  void Initialize();
  void LoadModules();
  void Process();
  void Finish();

 private:
   std::map<std::string, std::ostream *> propfiles;
   std::vector<lpmd::SimulationCell> configs;
   std::vector<lpmd::InstantProperty *> iproplist;
   std::vector<lpmd::TemporalProperty *> tproplist;
};

