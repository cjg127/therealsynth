#pragma once

#include "synthdefs.h"
#include "synthlfo.h"

enum {
	kRotorXOutput,
	kRotorYOutput
};

struct RotorParameters
{
	RotorParameters() {}
	RotorParameters& operator=(const RotorParameters& params)	// need this override for collections to work
	{
		if (this == &params)
			return *this;

		frequency = params.frequency;
		xAmp = params.xAmp;
		yAmp = params.yAmp;
		xLFOParameters = params.xLFOParameters;
		yLFOParameters = params.yLFOParameters;
		return *this;
	}

	// --- individual parameters
	double frequency = 2.0;
	double xAmp = 1.0;
	double yAmp = 1.0; 
	std::shared_ptr<SynthLFOParameters> xLFOParameters = std::make_shared<SynthLFOParameters>();
	std::shared_ptr<SynthLFOParameters> yLFOParameters = std::make_shared<SynthLFOParameters>();
};

/**
\class Rotor
\brief Rotor for ouptut of Circular Motion

Outputs: contains 2 outputs
- kXOutput
- kYOutput
*/

class Rotor : public ISynthModulator
{
public:
	Rotor(const std::shared_ptr<MidiInputData> _midiInputData, std::shared_ptr<RotorParameters> _parameters)
		: midiInputData(_midiInputData)
		, parameters(_parameters) {
		srand(time(NULL)); // --- seed random number generator
		xLFO.reset(new SynthLFO(midiInputData, parameters->xLFOParameters));
		yLFO.reset(new SynthLFO(midiInputData, parameters->yLFOParameters));

	}	

	virtual ~Rotor() {}

	virtual bool reset(double _sampleRate)
	{
		sampleRate = _sampleRate;
		xLFO->reset(_sampleRate);
		yLFO->reset(_sampleRate);
		xLFOOutput.clear();
		yLFOOutput.clear();
		parameters->xLFOParameters->lfoShape = LFOWaveform::kSin;
		parameters->yLFOParameters->lfoShape = LFOWaveform::kSin;
		return true;
	}

	virtual bool update(bool updateAllModRoutings = true);

	const ModOutputData renderModulatorOutput();

	virtual bool doNoteOn(double midiPitch, uint32_t _midiNoteNumber, uint32_t midiNoteVelocity)
	{
		xLFO->doNoteOn(midiPitch, _midiNoteNumber, midiNoteVelocity);
		yLFO->doNoteOn(midiPitch, _midiNoteNumber, midiNoteVelocity);
		return true;
	}

	virtual bool doNoteOff(double midiPitch, uint32_t _midiNoteNumber, uint32_t midiNoteVelocity)
	{
		xLFO->doNoteOff(midiPitch, _midiNoteNumber, midiNoteVelocity);
		yLFO->doNoteOff(midiPitch, _midiNoteNumber, midiNoteVelocity);
		return true;
	}

	virtual std::shared_ptr<ModInputData> getModulators() {
		return modulators;
	}

	virtual void setModulators(std::shared_ptr<ModInputData> _modulators) {
		modulators = _modulators;
	}

protected:
	const std::shared_ptr<MidiInputData> midiInputData = nullptr;
	std::shared_ptr<ModInputData> modulators = std::make_shared<ModInputData>();
	std::shared_ptr<RotorParameters> parameters = nullptr;
	
	ModOutputData xLFOOutput;
	ModOutputData yLFOOutput;

	std::unique_ptr<SynthLFO> xLFO;
	std::unique_ptr<SynthLFO> yLFO;

	


	double sampleRate = 0.0;
};