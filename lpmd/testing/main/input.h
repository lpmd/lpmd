//
//
//

#ifndef __LPMDMAIN_INPUT_H__
#define __LPMDMAIN_INPUT_H__

#include "../common/input.h"

#include <lpmd/atom.h>
#include <lpmd/paramlist.h>
#include <lpmd/applicable.h>
#include <lpmd/util.h>

class AtomTypeApplyInfo: public lpmd::IApplicable
{
 public:
   std::string name;
   lpmd::AtomType * t;
   long from_index, to_index; 
};

class MonitorApplyInfo: public lpmd::IApplicable
{
 public:
   std::string output;
   std::list<std::string> properties;
   std::ostream * mout;
   //
   MonitorApplyInfo(std::string props, long int start, long int end, long int each, std::string out)
   {
    properties = lpmd::ListOfTokens(props, ',');
    output = out;
    start_step = start;
    end_step = end;
    interval = each;
    mout = &(std::cout);
   }
};

class LPMDInputReader: public CommonInputReader
{
 public:
   LPMDInputReader(lpmd::PluginManager & pm, lpmd::Map & params);
   void Read(std::string sysfile, const lpmd::ParamList & optvars);
   int OnStatement(const std::string & name, const std::string & keywords, bool regular);
   
   std::list<AtomTypeApplyInfo> atapply;
   std::list<MonitorApplyInfo> monapply;

 private:
   bool inside_typeblock;
};

#endif

