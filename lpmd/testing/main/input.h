//
//
//

#ifndef __LPMDMAIN_INPUT_H__
#define __LPMDMAIN_INPUT_H__

#include "../common/input.h"

#include <lpmd/atom.h>
#include <lpmd/applicable.h>

class AtomTypeApplyInfo: public lpmd::IApplicable
{
 public:
   std::string name;
   lpmd::AtomType * t;
   long from_index, to_index; 
};

class LPMDInputReader: public CommonInputReader
{
 public:
   LPMDInputReader(lpmd::PluginManager & pm);
   void Read(std::string sysfile, const ParamList & optvars);
   int OnStatement(const std::string & name, const std::string & keywords, bool regular);
   
   std::list<AtomTypeApplyInfo> atapply;

 private:
   bool inside_typeblock;
};

#endif

