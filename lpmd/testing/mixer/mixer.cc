//
//
//

#include "mixer.h"
#include "cmdline.h"
#include "config.h"

#include <lpmd/session.h>
#include <lpmd/util.h>
#include <lpmd/containable.h>
#include <lpmd/cellformat.h>
#include <lpmd/cellmanager.h>
#include <lpmd/simulationcell.h>
#include <lpmd/systemmixer.h>

using namespace lpmd;

Mixer::Mixer(): CommonHandler("lpmd-mixer", "LPMD Mixer")
{
 CommonInputReader * inp = new MixerInputReader(pluginman, GlobalSession);
 SetInputReader(*inp);
}

Mixer::~Mixer()
{
 CommonInputReader & param = GetInputReader();
 CommonInputReader * inp = &param;
 delete inp;
}

void Mixer::Initialize()
{


}

void Mixer::LoadModules()
{
 CommonHandler::LoadModules();
 CommonInputReader & param = GetInputReader();
 //
 // Carga modulos SystemMixer
 // 
 std::vector<std::string> mixers = SplitTextLine(param["apply-list"]);
 for (unsigned int i=0;i<mixers.size();++i)
 {
  Module & pmod = pluginman[mixers[i]];
  SystemMixer & smix = CastModule<SystemMixer>(pmod);
  smlist.push_back(&smix);
  pmod.SetUsed();
 }
}

void Mixer::Process()
{


}

void Mixer::Finish()
{



}

//
//
//
int main(int argc, char *argv[])
{
 Mixer handler;
 MixerCmdLineParser clp(argc, argv);
 try { handler.Execute(clp); }
 catch (std::exception & ex) { EndWithError(ex.what()); }
 return 0;
}

