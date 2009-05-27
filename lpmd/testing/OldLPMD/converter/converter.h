//
//
//

#include "../common/handler.h"
#include "input.h"

#include <lpmd/systemmodifier.h>

#include <map>
#include <vector>

class Converter: public CommonHandler
{
 public:
  // Constructor y destructor 
  Converter();
  ~Converter();

  void Initialize();
  void LoadModules();
  void Process();
  void Finish();

 private:
   std::map<std::string, std::ostream *> propfiles;
   std::vector<lpmd::SimulationCell> configs;
   std::vector<lpmd::SystemModifier *> smlist;
};

