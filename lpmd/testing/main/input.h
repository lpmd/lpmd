//
//
//

#ifndef __LPMDMAIN_INPUT_H__
#define __LPMDMAIN_INPUT_H__

#include "../common/input.h"

#include <lpmd/atom.h>
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

class RunningAverageApplyInfo: public MonitorApplyInfo
{
 public:
   long int average_over;
   //
   RunningAverageApplyInfo(std::string props, long int start, long int end, long int each, std::string out, long int over): MonitorApplyInfo(props, start, end, each, out), average_over(over), average(0.0) { }

   void AddNewValue(double v)
   {
    if (buffer.size() >= average_over) 
    {
     double f = buffer.front();
     buffer.pop_front();
     average = average + ((f-v)/double(average_over));
    }
    else
    {
     average = average*buffer.size()+v;
     average /= double(1+buffer.size());
    }
    buffer.push_back(v);
   }

   double Average() { return average; } 

 private:
   std::list<double> buffer;
   double average;
};

class LPMDInputReader: public CommonInputReader
{
 public:
   LPMDInputReader(lpmd::PluginManager & pm);
   void Read(std::string sysfile, const ParamList & optvars);
   int OnStatement(const std::string & name, const std::string & keywords, bool regular);
   
   std::list<AtomTypeApplyInfo> atapply;
   std::list<MonitorApplyInfo> monapply;
   std::list<RunningAverageApplyInfo> ravapply;

 private:
   bool inside_typeblock;
};

#endif

