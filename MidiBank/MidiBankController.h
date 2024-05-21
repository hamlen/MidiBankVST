#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

// Plugin controller GUID - must be unique
static const FUID MidiBankControllerUID(0x6bf5a312,0xa36b4bd3,0xac9ef2dd,0x0cfb9fba);

class MidiBankController : public EditControllerEx1, public IMidiMapping
{
public:
	MidiBankController(void);

	static FUnknown* createInstance(void* context)
	{
		return (IEditController*) new MidiBankController;
	}

	DELEGATE_REFCOUNT(EditControllerEx1)
	tresult PLUGIN_API queryInterface(const char* iid, void** obj) SMTG_OVERRIDE;

	tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate() SMTG_OVERRIDE;
	tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE;
	tresult PLUGIN_API getMidiControllerAssignment(int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& id) SMTG_OVERRIDE;

	~MidiBankController(void);
};

