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

   // Overrides virtual method from lpmd::InputFile 
   void Read(std::istream & istr, const ParamList & options, const std::string inpfile);

   virtual void Read(std::string sysfile, const ParamList & optvars);
   virtual int OnStatement(const std::string & name, const std::string & keywords, bool regular);

   void ParseLine(std::string);

   int CheckConsistency();

   //
   std::list<lpmd::ModuleInfo> uselist;
   std::list<lpmd::ModuleInfo> preparelist;
   std::list<lpmd::ModuleInfo> outputlist;
   std::map<std::string, double> bondtable;

 protected:
   const ParamList * ovpointer;
   lpmd::PluginManager * pluginman;
};

#endif

