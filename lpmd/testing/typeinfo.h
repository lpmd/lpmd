/*
 *
 *
 *
 */

#ifndef __LPMDUTIL_TYPEINFO_H__
#define __LPMDUTIL_TYPEINFO_H__

#include <lpmd/paramlist.h>

using namespace lpmd;

class TypeInfo: public lpmd::ParamList
{
 public:
   std::string name;

 TypeInfo(): ParamList(), name() { }
 TypeInfo(const TypeInfo & t): ParamList(t), name(t.name) { }

 TypeInfo & operator=(const TypeInfo & t)
 {
  if (&t != this)
  {
   ParamList::operator=(t);
   name = t.name;
  }
  return (*this);
 }

};

#endif

