#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"
#include <pluginterfaces/vst/ivstmidicontrollers.h>

#include "MidiBank.h"
#include "MidiBankController.h"
#include <string>
#include <pluginterfaces/base/ustring.h>

MidiBankController::MidiBankController(void)
{
	LOG("MidiBankController constructor called and exited.\n");
}

MidiBankController::~MidiBankController(void)
{
	LOG("MidiBankController destructor called and exited.\n");
}

tresult PLUGIN_API MidiBankController::queryInterface(const char* iid, void** obj)
{
	QUERY_INTERFACE(iid, obj, IMidiMapping::iid, IMidiMapping)
	return EditControllerEx1::queryInterface(iid, obj);
}

tresult PLUGIN_API MidiBankController::initialize(FUnknown* context)
{
	LOG("MidiBankController::initialize called.\n");
	tresult result = EditControllerEx1::initialize(context);

	if (result != kResultOk)
	{
		LOG("MidiBankController::initialize exited prematurely with code %d.\n", result);
		return result;
	}

	RangeParameter* bank_param = new RangeParameter(STR16("Bank"), 0, nullptr, 1., 16., 1., 15, ParameterInfo::kCanAutomate);
	bank_param->setPrecision(0);
	parameters.addParameter(bank_param);
	char16_t pname[7] = STR16("00:000");
	for (uint32 c = 0; c < num_channels; ++c)
	{
		pname[0] = u'0' + (c + 1) / 10;
		pname[1] = u'0' + (c + 1) % 10;
		for (uint32 cc = 0; cc < num_controllers; ++cc)
		{
			pname[3] = u'0' + (cc / 100) % 10;
			pname[4] = u'0' + (cc / 10) % 10;
			pname[5] = u'0' + cc % 10;
			RangeParameter* p = new RangeParameter(pname, num_exposed_params + c * num_controllers + cc, nullptr, 0., 127., 0., 127, ParameterInfo::kCanAutomate);
			p->setPrecision(0);
			parameters.addParameter(p);
		}
	}

	LOG("MidiBankController::initialize exited normally with code %d.\n", result);
	return result;
}

tresult PLUGIN_API MidiBankController::terminate()
{
	LOG("MidiBankController::terminate called.\n");
	tresult result = EditControllerEx1::terminate();
	LOG("MidiBankController::terminate exited with code %d.\n", result);
	return result;
}

tresult PLUGIN_API MidiBankController::setComponentState(IBStream* state)
{
	LOG("MidiBankController::setComponentState called.\n");
	if (!state)
	{
		LOG("MidiBankController::setComponentState failed because no state argument provided.\n");
		return kResultFalse;
	}

	IBStreamer streamer(state, kLittleEndian);
	ParamValue val;
	if (!streamer.readDouble(val))
	{
		LOG("MidiBankController::setComponentState failed.\n");
		return kResultOk;
	}
	setParamNormalized(0, val);

	LOG("MidiBankController::setComponentState exited normally.\n");
	return kResultOk;
}

tresult PLUGIN_API MidiBankController::getMidiControllerAssignment(int32 busIndex, int16 midiChannel, CtrlNumber midiControllerNumber, ParamID& tag)
{
	LOG("MidiBankController::getMidiControllerAssignment called.\n");
	if (busIndex == 0 && 0 <= midiChannel && midiChannel < num_channels && 0 <= midiControllerNumber && midiControllerNumber < num_controllers)
	{
		tag = (ParamID)(num_exposed_params + midiChannel * num_controllers + midiControllerNumber);
		LOG("MidiBankController::getMidiControllerAssignment exited normally.\n");
		return kResultTrue;
	}
	LOG("MidiBankController:getMidiControllerAssignment exited with failure.\n");
	return kResultFalse;
}
