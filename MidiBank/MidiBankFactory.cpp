#include "MidiBank.h"
#include "MidiBankController.h"

#include "public.sdk/source/main/pluginfactory.h"

#define PluginCategory "Fx"
#define PluginName "MidiBank"

#define PLUGINVERSION "1.0.0"

bool InitModule()
{
	LOG("InitModule called and exited.\n");
	return true;
}

bool DeinitModule()
{
	LOG("DeinitModule called and exited.\n");
	return true;
}

BEGIN_FACTORY_DEF("Kevin Hamlen",
	"https://github.com/hamlen/MidiBankVST",
	"no contact")

	LOG("GetPluginFactory called.\n");

	DEF_CLASS2(INLINE_UID_FROM_FUID(MidiBankProcessorUID),
		PClassInfo::kManyInstances,
		kVstAudioEffectClass,
		PluginName,
		Vst::kDistributable,
		PluginCategory,
		PLUGINVERSION,
		kVstVersionString,
		MidiBank::createInstance)

	DEF_CLASS2(INLINE_UID_FROM_FUID(MidiBankControllerUID),
		PClassInfo::kManyInstances,
		kVstComponentControllerClass,
		PluginName "Controller",
		0,  // unused
		"", // unused
		PLUGINVERSION,
		kVstVersionString,
		MidiBankController::createInstance)

END_FACTORY
