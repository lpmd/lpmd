//
//
//

#ifndef __LPMD_INPUT_H__
#define __LPMD_INPUT_H__

#include <list>
#include <map>
#include <exception>

#include <lpmd/inputfile.h>
#include <lpmd/pluginmanager.h>

class LPMDInputReader: public lpmd::InputFile
{
 public:
   LPMDInputReader(lpmd::PluginManager & pm);
   LPMDInputReader(std::string sysfile);
   ~LPMDInputReader();

   void Read(std::string sysfile);
   int OnStatement(const std::string & name, const std::string & keywords, bool regular);

   int CheckConsistency();

   //
   std::list<lpmd::ModuleInfo> uselist;

 private:
   lpmd::PluginManager * pluginman;
   bool inside_useblock, inside_typeblock;
};

#endif

