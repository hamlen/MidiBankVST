#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"

#include "MidiBank.h"
#include "MidiBankController.h"

MidiBank::MidiBank(void)
{
	LOG("MidiBank constructor called.\n");
	setControllerClass(FUID(MidiBankControllerUID));
	processSetup.maxSamplesPerBlock = INT32_MAX;
	LOG("MidiBank constructor exited.\n");
}

MidiBank::~MidiBank(void)
{
	LOG("MidiBank destructor called and exited.\n");
}

tresult PLUGIN_API MidiBank::initialize(FUnknown* context)
{
	LOG("MidiBank::initialize called.\n");
	tresult result = AudioEffect::initialize(context);

	if (result != kResultOk)
	{
		LOG("MidiBank::initialize failed with code %d.\n", result);
		return result;
	}

	addEventInput(STR16("Event In"));
	addEventOutput(STR16("Event Out"));

	LOG("MidiBank::initialize exited normally.\n");
	return kResultOk;
}

tresult PLUGIN_API MidiBank::terminate()
{
	LOG("MidiBank::terminate called.\n");
	tresult result = AudioEffect::terminate();
	LOG("MidiBank::terminate exited with code %d.\n", result);
	return result;
}

tresult PLUGIN_API MidiBank::setActive(TBool state)
{
	LOG("MidiBank::setActive called.\n");
	tresult result = AudioEffect::setActive(state);
	LOG("MidiBank::setActive exited with code %d.\n", result);
	return result;
}

tresult PLUGIN_API MidiBank::setIoMode(IoMode mode)
{
	LOG("MidiBank::setIoMode called and exited.\n");
	return kResultOk;
}

tresult PLUGIN_API MidiBank::setProcessing(TBool state)
{
	LOG("MidiBank::setProcessing called and exited.\n");
	return kResultOk;
}

tresult PLUGIN_API MidiBank::setState(IBStream* state)
{
	LOG("MidiBank::setState called.\n");

	IBStreamer streamer(state, kLittleEndian);
	int8 val;
	if (!streamer.readInt8(val))
	{
		LOG("MidiBank::setState failed.\n");
		return kResultOk;
	}
	channel = val;

	LOG("MidiBank::setState exited successfully.\n");
	return kResultOk;
}

tresult PLUGIN_API MidiBank::getState(IBStream* state)
{
	LOG("MidiBank::getState called.\n");

	IBStreamer streamer(state, kLittleEndian);
	if (!streamer.writeInt8(channel))
	{
		LOG("MidiBank::getState failed due to streamer error.\n");
		return kResultFalse;
	}

	LOG("MidiBank::getState exited successfully.\n");
	return kResultOk;
}

tresult PLUGIN_API MidiBank::setupProcessing(ProcessSetup& newSetup)
{
	LOG("MidiBank::setupProcessing called.\n");
	processContextRequirements.flags = 0;
	tresult result = AudioEffect::setupProcessing(newSetup);
	LOG("MidiBank::setupProcessing exited with code %d.\n", result);
	return result;
}

tresult PLUGIN_API MidiBank::canProcessSampleSize(int32 symbolicSampleSize)
{
	LOG("MidiBank::canProcessSampleSize called with arg %d and exited.\n", symbolicSampleSize);
	return (symbolicSampleSize == kSample32 || symbolicSampleSize == kSample64) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API MidiBank::getRoutingInfo(RoutingInfo& inInfo, RoutingInfo& outInfo)
{
	LOG("MidiBank::getRoutingInfo called.\n");
	if (inInfo.mediaType == kEvent && inInfo.busIndex == 0)
	{
		outInfo = inInfo;
		LOG("MidiBank::getRoutingInfo exited with success.\n");
		return kResultOk;
	}
	else
	{
		LOG("MidiBank::getRoutingInfo exited with failure.\n");
		return kResultFalse;
	}
}

static inline int32 discretize(ParamValue value, int32 max_value)
{
	const int32 discrete = (int32)(value * (ParamValue)(max_value + 1));
	return (discrete < 0) ? 0 : ((discrete > max_value) ? max_value : discrete);
}

tresult PLUGIN_API MidiBank::process(ProcessData& data)
{
	if (!data.processContext || data.numSamples < 0)
	{
		LOG("MidiBank::process aborted due to bad sample rate provided by host.\n");
		return kResultFalse;
	}

	// We shouldn't be asked for any audio, but process it anyway (emit silence) to tolerate uncompliant hosts.
	for (int32 i = 0; i < data.numOutputs; ++i)
	{
		for (int32 j = 0; j < data.outputs[i].numChannels; ++j)
		{
			bool is32bit = (data.symbolicSampleSize == kSample32);
			if (is32bit || (data.symbolicSampleSize == kSample64))
			{
				void* channelbuffer = is32bit ? (void*)data.outputs[i].channelBuffers32[j] : (void*)data.outputs[i].channelBuffers64[j];
				size_t datasize = is32bit ? sizeof(*data.outputs[i].channelBuffers32[j]) : sizeof(*data.outputs[i].channelBuffers64[j]);
				if (channelbuffer)
					memset(channelbuffer, 0, data.numSamples * datasize);
			}
		}
		data.outputs[i].silenceFlags = (1ULL << data.outputs[i].numChannels) - 1;
	}

	if (data.numSamples <= 0)
		return kResultTrue;

	const int32 numInParamsChanged = data.inputParameterChanges ? data.inputParameterChanges->getParameterCount() : 0;
	const int32 numInEvents = data.inputEvents ? data.inputEvents->getEventCount() : 0;
	if (numInParamsChanged > 0 || numInEvents > 0)
	{
		// Organize host-provided parameter change queues into arrays.
		IParamValueQueue** in_queue = nullptr;
		int32* pcounter = nullptr;
		int32 ecounter = 0;
		if (numInParamsChanged > 0)
		{
			if (!((in_queue = (IParamValueQueue**)calloc(numInParamsChanged, sizeof(IParamValueQueue*)))))
				return kResultFalse;
			if (!((pcounter = (int32*)calloc(numInParamsChanged, sizeof(int32)))))
			{
				free(in_queue);
				return kResultFalse;
			}
			for (int32 i = 0; i < numInParamsChanged; ++i)
				in_queue[i] = data.inputParameterChanges->getParameterData(i);
		}

		for (;;)
		{
			// Find next parameter change.
			int32 sample = data.numSamples;
			int32 next_queue_index = -1;
			ParamValue new_value;
			Event e;
			if (ecounter < numInEvents)
			{
				data.inputEvents->getEvent(ecounter, e);
				sample = e.sampleOffset;
				next_queue_index = numInParamsChanged;
			}
			for (int32 i = 0; i < numInParamsChanged; ++i)
			{
				if (pcounter[i] < in_queue[i]->getPointCount())
				{
					int32 x;
					ParamValue y;
					if ((in_queue[i]->getPoint(pcounter[i], x, y) == kResultOk) && (x < sample))
					{
						sample = x;
						new_value = y;
						next_queue_index = i;
					}
				}
			}
			if (next_queue_index < 0)
				break;

			if (next_queue_index < numInParamsChanged)
			{
				// Parameter changed
				IParamValueQueue* const q = in_queue[next_queue_index];
				++pcounter[next_queue_index];

				const ParamID id = q->getParameterId();
				if (id < num_exposed_params)
				{
					// Bank change
					const int8 new_channel = discretize(new_value, num_channels - 1);
					if ((new_channel != channel) && data.outputEvents)
					{
						Event e = {};
						e.type = Event::EventTypes::kLegacyMIDICCOutEvent;
						e.sampleOffset = sample;
						e.midiCCOut.channel = new_channel;
						for (int32 cc = 0; cc < num_controllers; ++cc)
						{
							if (midivals[channel][cc] != midivals[new_channel][cc])
							{
								e.midiCCOut.controlNumber = cc;
								e.midiCCOut.value = midivals[new_channel][cc];
								data.outputEvents->addEvent(e);
							}
						}
						channel = new_channel;
					}
				}
				else if (id < num_exposed_params + num_channels * num_controllers)
				{
					// CC value changed
					const uint32 chan = (id - num_exposed_params) / num_controllers;
					const uint32 cc = (id - num_exposed_params) % num_controllers;
					const int32 y = discretize(new_value, 127);
					midivals[chan][cc] = y;
					if ((chan == channel) && data.outputEvents)
					{
						Event e = {};
						e.type = Event::EventTypes::kLegacyMIDICCOutEvent;
						e.sampleOffset = sample;
						e.midiCCOut.channel = chan;
						e.midiCCOut.controlNumber = cc;
						e.midiCCOut.value = y;
						data.outputEvents->addEvent(e);
					}
				}
			}
			else
			{
				// Event message received
				++ecounter;
				if ((e.type == Event::EventTypes::kDataEvent) && (e.data.type == DataEvent::kMidiSysEx) &&
					(e.data.size >= 3) && (e.data.bytes[0] == 0xF0) && (e.data.bytes[e.data.size - 1] == 0xF7))
				{
					Event e = {};
					e.type = Event::EventTypes::kLegacyMIDICCOutEvent;
					e.sampleOffset = sample;
					e.midiCCOut.channel = channel;
					if (e.data.bytes[1])
					{
						// sysex message F0 01 mm mm ... F7 = retransmit selected cc values of current bank
						if (data.outputEvents)
						{
							uint32 max_data_size = 2 + (num_controllers + 6) / 7 + 1;
							if (e.data.size < max_data_size) max_data_size = e.data.size;
							for (uint32 i = 0; 2 + i < max_data_size; ++i)
							{
								uint8 byte = e.data.bytes[2 + i];
								if (byte >= 0x80)
									break;
								for (uint32 cc = i * 7; byte && (cc < num_controllers); byte >>= 1, ++cc)
								{
									if (byte & 1)
									{
										e.midiCCOut.controlNumber = cc;
										e.midiCCOut.value = midivals[channel][cc];
										data.outputEvents->addEvent(e);
									}
								}
							}
						}
					}
					else
					{
						// sysex message F0 00 F7 = zero all banks
						if (data.outputEvents)
						{
							e.midiCCOut.value = 0;
							for (uint32 cc = 0; cc < num_controllers; ++cc)
							{
								if (midivals[channel][cc])
								{
									e.midiCCOut.controlNumber = cc;
									data.outputEvents->addEvent(e);
								}
							}
						}
						const int32 numOutParamsChanged = data.outputParameterChanges ? data.outputParameterChanges->getParameterCount() : 0;
						for (int32 i = 0; i < numOutParamsChanged; ++i)
						{
							IParamValueQueue* q = data.outputParameterChanges->getParameterData(i);
							const ParamID id = q->getParameterId();
							if (id >= num_exposed_params)
							{
								const uint32 chan = (id - num_exposed_params) / num_controllers;
								const uint32 cc = (id - num_exposed_params) % num_controllers;
								if (midivals[chan][cc])
								{
									int32 dummy;
									q->addPoint(sample, 0., dummy);
									midivals[chan][cc] = 0;
								}
							}
						}
						for (uint32 chan = 0; chan < num_channels; ++chan)
						{
							for (uint32 cc = 0; cc < num_controllers; ++cc)
							{
								if (midivals[chan][cc])
								{
									int32 dummy;
									IParamValueQueue* q = data.outputParameterChanges->addParameterData(num_exposed_params + chan * num_controllers + cc, dummy);
									if (q)
										q->addPoint(sample, 0., dummy);
									midivals[chan][cc] = 0;
								}
							}
						}
					}
				}
			}
		}

		free(pcounter);
		free(in_queue);
	}

	return kResultOk;
}
