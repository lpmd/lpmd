//
//
//

#ifndef __LPMDCOMMON_INPUT_H__
#define __LPMDCOMMON_INPUT_H__

#include <list>
#include <map>
#include <exception>

#include <lpmd/inputfile.h>
#include <lpmd/pluginmanager.h>

class CommonInputReader: public lpmd::InputFile
{
 public:
   CommonInputReader(lpmd::PluginManager & pm);
   ~CommonInputReader();

   virtual void Read(std::string sysfile, const ParamList & optvars);
   virtual int OnStatement(const std::string & name, const std::string & keywords, bool regular);
   void ParseLine(std::string);

   int CheckConsistency();

   //
   std::list<lpmd::ModuleInfo> uselist;
   std::list<lpmd::ModuleInfo> preparelist;
   std::list<lpmd::ModuleInfo> outputlist;

 protected:
   const ParamList * ovpointer;
   lpmd::PluginManager * pluginman;
   bool inside_useblock;
};

#endif

