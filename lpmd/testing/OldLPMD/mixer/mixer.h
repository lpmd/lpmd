//
//
//

#include "../common/handler.h"
#include "input.h"

#include <lpmd/systemmixer.h>
#include <vector>

class Mixer: public CommonHandler
{
 public:
  // Constructor y destructor 
  Mixer();
  ~Mixer();

  void Initialize();
  void LoadModules();
  void Process();
  void Finish();
  
 private: 
  std::vector<lpmd::SystemMixer *> smlist;
};

