/*
 *
 *
 *
 */

#include "application.h"
#include "config.h"
#include <lpmd/session.h>

#include <iostream>
#include <iomanip>

void Application::PrintBanner(const std::string & text)
{
 std::cout << std::setfill('*');
 std::cout << std::endl;
 std::cout << std::setw(80) << "" << std::endl;
 std::cout << "* " << std::setw(78) << std::left << text+" " << std::endl;
 std::cout << std::setw(80) << "" << std::endl;
 std::cout << std::setfill(' ');
}

void Application::ShowHelp()
{
 std::cerr << name << " version " << VERSION;
 std::cerr << '\n';
 std::cerr << "Using liblpmd version " << lpmd::GlobalSession["libraryversion"] << std::endl << std::endl;
 std::cerr << "Usage: " << cmdname << " [--verbose | -v ] [--lengths | -L <a,b,c>] [--angles | -A <alpha,beta,gamma>]";
 std::cerr << " [--vector | -V <ax,ay,az,bx,by,bz,cx,cy,cz>] [--scale | -S <value>]";
 std::cerr << " [--option | -O <option=value,option=value,...>] [--input | -i plugin:opt1,opt2,...] [--output | -o plugin:opt1,opt2,...]";
 std::cerr << " [--use | -u plugin:opt1,opt2,...] [--cellmanager | -c plugin:opt1,opt2,...] [--replace-cell | -r] [file.control]\n";
 std::cerr << "       " << cmdname << " [--pluginhelp | -p <pluginname>]\n";
 std::cerr << "       " << cmdname << " [--help | -h]\n";
 exit(1);
}

void Application::ShowPluginHelp()
{
 std::cout << "Loaded from file: " << pluginmanager["help_plugin"]["fullpath"] << '\n';
 if (pluginmanager["help_plugin"].Defined("version")) 
    std::cout << "Plugin version: " << pluginmanager["help_plugin"]["version"] << '\n';
 std::cout << '\n';
 pluginmanager["help_plugin"].ShowHelp();
 PrintBanner("Provides");
 std::cout << "     >> " << pluginmanager["help_plugin"].Provides() << '\n';
 PrintBanner("Arguments Required or supported");
 std::cout << "     >> " << pluginmanager["help_plugin"].Keywords() << '\n';
 PrintBanner("Default values for parameters");
 pluginmanager["help_plugin"].Show(std::cout);
 exit(1);
}

