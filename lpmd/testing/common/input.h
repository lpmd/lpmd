//
//
//

#ifndef __LPMDCOMMON_INPUT_H__
#define __LPMDCOMMON_INPUT_H__

#include <list>
#include <map>
#include <exception>

#include <lpmd/paramlist.h>
#include <lpmd/inputfile.h>
#include <lpmd/pluginmanager.h>

class CommonInputReader: public lpmd::InputFile
{
 public:
   CommonInputReader(lpmd::PluginManager & pm, lpmd::Map & params);
   ~CommonInputReader();

   // Overrides virtual method from lpmd::InputFile 
   void Read(std::istream & istr, const lpmd::ParamList & options, const std::string inpfile);

   virtual void Read(std::string sysfile, const lpmd::ParamList & optvars);
   virtual int OnStatement(const std::string & name, const std::string & keywords, bool regular);

   void ParseLine(std::string);

   int CheckConsistency();

   //
   std::list<lpmd::ModuleInfo> uselist;
   std::list<lpmd::ModuleInfo> preparelist;
   std::list<lpmd::ModuleInfo> outputlist;
   std::map<std::string, double> bondtable;

 protected:
   const lpmd::ParamList * ovpointer;
   lpmd::PluginManager * pluginman;
};

#endif

