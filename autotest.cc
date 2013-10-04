/*
 *
 *
 *
 */

#include "application.h"
#include <iostream>

void Application::AutoTestPlugin(const std::string & pluginname)
{
 bool result;
 pluginmanager.LoadPlugin(pluginname, "autotested", "");
 try { result = pluginmanager["autotested"].AutoTest(); }
 catch (std::exception & e) 
 { 
  std::cerr << "[Error] " << e.what() << '\n';
  result = false; 
 }
 if (result) std::cerr << "\n-> " << pluginname << " OK\n";
 else std::cerr << "\n-> " << pluginname << " FAILED\n";
 exit(result ? 0 : 1);
}

