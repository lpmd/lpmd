/*
 *
 *
 *
 */

#ifndef __LPMDCOMMON_HANDLER_H__
#define __LPMDCOMMON_HANDLER_H__

#include "input.h"
#include "cmdline.h"

#include <lpmd/md.h>
#include <lpmd/paramlist.h>
#include <lpmd/pluginmanager.h>

class CommonHandler: public lpmd::MD
{
 public:
  // Constructor y destructor 
  CommonHandler(const std::string & cmdnam, const std::string & fullnam);
  virtual ~CommonHandler();

  virtual void BannerPrint(std::string);
  virtual void ShowHelp();
  virtual void ShowPluginHelp(std::string);
  virtual void LoadUseModules();
  virtual void LoadModules();

  virtual void Initialize() = 0;
  virtual void Process() = 0;
  virtual void Finish() = 0;

  virtual void Execute(CommonCmdLineParser & clp);

  void SetOptionVariables(CommonCmdLineParser & clp, lpmd::ParamList & ov);
  void SetInputReader(CommonInputReader & cir) { parm = &cir; }
  CommonInputReader & GetInputReader() const { return (*parm); }

 protected:
   lpmd::SimulationCell *scell;
   lpmd::PluginManager pluginman;

 private:
   std::string cmdname, name;
   CommonInputReader * parm;
};

#endif

