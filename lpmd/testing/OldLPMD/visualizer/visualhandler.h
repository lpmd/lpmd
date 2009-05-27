//
//
//

#include "../common/handler.h"
#include "input.h"

#include <lpmd/visualizer.h>

#include <map>
#include <vector>

class VisualHandler: public CommonHandler
{
 public:
  // Constructor y destructor 
  VisualHandler();
  ~VisualHandler();

  void Initialize();
  void LoadModules();
  void Process();
  void ProcessConfig();
  void Finish();

 private:
   std::vector<lpmd::Visualizer *> vislist;
};

