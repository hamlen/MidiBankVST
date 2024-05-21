#pragma once

#undef LOGGING

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "base/source/fstring.h"
#include "pluginterfaces/base/funknown.h"
#include <pluginterfaces/vst/ivstparameterchanges.h>

using namespace Steinberg;
using namespace Steinberg::Vst;

// Plugin processor GUID - must be unique
static const FUID MidiBankProcessorUID(0x347c7c92,0x21a94326,0x860dbbac, 0xdbdcb785);

constexpr uint32 num_exposed_params = 1;
constexpr uint32 num_channels = 16;
constexpr uint32 num_controllers = 128;

class MidiBank : public AudioEffect
{
public:
	MidiBank(void);

	static FUnknown* createInstance(void* context)
	{
		return (IAudioProcessor*) new MidiBank();
	}

	tresult PLUGIN_API initialize(FUnknown* context);
	tresult PLUGIN_API terminate();
	tresult PLUGIN_API setupProcessing(ProcessSetup& newSetup);
	tresult PLUGIN_API setActive(TBool state);
	tresult PLUGIN_API setProcessing(TBool state);
	tresult PLUGIN_API process(ProcessData& data);
	tresult PLUGIN_API getRoutingInfo(RoutingInfo& inInfo, RoutingInfo& outInfo);
	tresult PLUGIN_API setIoMode(IoMode mode);
	tresult PLUGIN_API setState(IBStream* state);
	tresult PLUGIN_API getState(IBStream* state);
	tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize);
	~MidiBank(void);

protected:
	int8 channel = 0;
	int8 midivals[num_channels][num_controllers] = {};
};

#ifdef LOGGING
	void log(const char* format, ...);
#	define LOG(format, ...) log((format), __VA_ARGS__)
#else
#	define LOG(format, ...) 0
#endif
