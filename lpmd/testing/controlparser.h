/*
 *
 *
 *
 */

#ifndef __LPMDUTIL_CONTROLPARSER_H__
#define __LPMDUTIL_CONTROLPARSER_H__

#include <lpmd/controlfile.h>
#include <lpmd/moduleinfo.h>

#include "typeinfo.h"

using namespace lpmd;

class UtilityControl: public ControlFile
{
 public:
   UtilityControl();
   virtual ~UtilityControl();

   int OnRegularStatement(const std::string & name, const ParamList & keywords);
   int OnNonRegularStatement(const std::string & name, const std::string & full_statement);
   int OnBlock(const std::string & name, const std::string & full_statement);
   
   Array<std::string> & PluginPath() const;
   Array<ModuleInfo> & Plugins() const;
   Array<TypeInfo> & Types() const;
   ParamList & Potentials() const;
   ParamList & Bonds() const;

 private:
   class UControlImpl * impl;
};

#endif

