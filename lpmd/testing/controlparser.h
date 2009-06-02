/*
 *
 *
 *
 */

#ifndef __LPMDUTIL_CONTROLPARSER_H__
#define __LPMDUTIL_CONTROLPARSER_H__

#include <lpmd/controlfile.h>
#include <lpmd/pluginmanager.h>
#include <lpmd/moduleinfo.h>

#include "typeinfo.h"

using namespace lpmd;

class UtilityControl: public ControlFile
{
 public:
   UtilityControl(PluginManager & pm);
   virtual ~UtilityControl();

   int OnRegularStatement(const std::string & name, const std::string & keywords);
   int OnNonRegularStatement(const std::string & name, const std::string & full_statement);
   int OnBlock(const std::string & name, const std::string & full_statement);
   
   Array<std::string> & PluginPath() const;
   Array<ModuleInfo> & Plugins() const;
   Array<TypeInfo> & Types() const;
   ParamList & Potentials() const;
   ParamList & Bonds() const;

   //
   ParamList massgroups, chargegroups;

 private:
   class UControlImpl * impl;
};

#endif

