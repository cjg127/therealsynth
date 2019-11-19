#include "synthlfo.h"


bool SynthLFO::update(bool updateAllModRoutings)
{
	// --- Run priority modulators 

	// --- End Priority modulators

	// TODO: put ramping update in this function
	if (!updateAllModRoutings)
		return true;

	// add modulation input here
	

	double range = (20.0 - 0.02) / 2.0;
	double bipolarFMod = modulators->modulationInputs[kFrequencyMod];
	double scaledMod = bipolarFMod * range;
	double total_freq = parameters->frequency_Hz + scaledMod;
	boundValue(total_freq, 0.02, 20.0);

	phaseInc = total_freq / sampleRate;
	
	// parameters->outputAmplitude = modulators->modulationInputs[kUnipolarMod];

	return true;
}

const ModOutputData SynthLFO::renderModulatorOutput()
{
	lfoTimer.advanceTimer();
	// --- setup for output
	ModOutputData lfoOutputData; // should auto-zero on instantiation
	lfoOutputData.clear();

	if (parameters->lfoRamp_mSec == 0.0)
	{
		rampMult = 1.0;
	}

	if (renderComplete)
	{
		return lfoOutputData;
	}

	if ((parameters->mode == LFOMode::kSync) || (parameters->mode == LFOMode::kOneShot))
	{
		if (!lfoTimer.timerExpired()) 
		{
			return lfoOutputData;
		}
	}

	// --- always first!
	bool bWrapped = checkAndWrapModulo(modCounter, phaseInc);
	if (bWrapped && parameters->mode == LFOMode::kOneShot)
	{
		renderComplete = true;
		return lfoOutputData;
	}

	// --- QP output always follows location of current modulo; first set equal
	modCounterQP = modCounter;

	// --- then, advance modulo by quadPhaseInc = 0.25 = 90 degrees, AND wrap if needed
	advanceAndCheckWrapModulo(modCounterQP, 0.25);

	// --- calculate the oscillator value
	if (parameters->lfoShape == LFOWaveform::kSin)
	{
		// --- calculate normal angle
		double angle = modCounter*2.0*kPi - kPi;

		// --- norm output with parabolicSine approximation
		lfoOutputData.modulationOutputs[kLFONormalOutput] = parabolicSine(-angle);

		// --- calculate QP angle
		angle = modCounterQP*2.0*kPi - kPi;

		// --- calc QP output
		lfoOutputData.modulationOutputs[kLFOQuadPhaseOutput] = parabolicSine(-angle);
	}
	else if (parameters->lfoShape == LFOWaveform::kTriangle)
	{
		// --- triv saw
		lfoOutputData.modulationOutputs[kLFONormalOutput] = unipolarToBipolar(modCounter);

		// --- bipolar triagle
		lfoOutputData.modulationOutputs[kLFONormalOutput] = 2.0*fabs(lfoOutputData.modulationOutputs[kLFONormalOutput]) - 1.0;

		// -- quad phase
		lfoOutputData.modulationOutputs[kLFOQuadPhaseOutput] = unipolarToBipolar(modCounterQP);

		// bipolar triagle
		lfoOutputData.modulationOutputs[kLFOQuadPhaseOutput] = 2.0*fabs(lfoOutputData.modulationOutputs[kLFOQuadPhaseOutput]) - 1.0;
	}
	else if (parameters->lfoShape == LFOWaveform::kSaw)
	{
		lfoOutputData.modulationOutputs[kLFONormalOutput] = unipolarToBipolar(modCounter);
		lfoOutputData.modulationOutputs[kLFOQuadPhaseOutput] = unipolarToBipolar(modCounterQP);
	}
	else if (parameters->lfoShape == LFOWaveform::kNoise)
	{
		lfoOutputData.modulationOutputs[kLFONormalOutput] = doWhiteNoise();
		lfoOutputData.modulationOutputs[kLFOQuadPhaseOutput] = doWhiteNoise();
	}
	else if (parameters->lfoShape == LFOWaveform::kQRNoise)
	{
		lfoOutputData.modulationOutputs[kLFONormalOutput] = doPNSequence(pnRegister);
		lfoOutputData.modulationOutputs[kLFOQuadPhaseOutput] = doPNSequence(pnRegister);
	}
	else if (parameters->lfoShape == LFOWaveform::kRSH || parameters->lfoShape == LFOWaveform::kQRSH)
	{
		// --- is this is the very first run? if so, form first output sample
		if (randomSHCounter < 0)
		{
			if (parameters->lfoShape == LFOWaveform::kRSH)
				randomSHValue = doWhiteNoise();
			else
				randomSHValue = doPNSequence(pnRegister);

			// --- init the sample counter, will be advanced below
			randomSHCounter = 1.0;
		}
		// --- has hold time been exceeded? if so, generate next output sample
		else if (randomSHCounter > (sampleRate / parameters->frequency_Hz))
		{
			// --- wrap counter
			randomSHCounter -= sampleRate / parameters->frequency_Hz;

			if (parameters->lfoShape == LFOWaveform::kRSH)
				randomSHValue = doWhiteNoise();
			else
				randomSHValue = doPNSequence(pnRegister);
		}

		// --- advance the sample counter
		randomSHCounter += 1.0;

		lfoOutputData.modulationOutputs[kLFONormalOutput] = randomSHValue;
		lfoOutputData.modulationOutputs[kLFOQuadPhaseOutput] = randomSHValue;
	}

	// --- timer clear
	if (parameters->mode == LFOMode::kFreeRun)
	{
		if (!lfoTimer.timerExpired())
		{
			lfoOutputData.clear();
		}
	}

	//


	// --- scale by amplitude
	lfoOutputData.modulationOutputs[kLFONormalOutput] *= parameters->outputAmplitude * rampMult;
	lfoOutputData.modulationOutputs[kLFOQuadPhaseOutput] *= parameters->outputAmplitude * rampMult;

	// --- invert two main outputs to make the opposite versions, scaling carries over
	lfoOutputData.modulationOutputs[kLFONormalOutputInverted] = -lfoOutputData.modulationOutputs[kLFONormalOutput];
	lfoOutputData.modulationOutputs[kLFOQuadPhaseOutputInverted] = -lfoOutputData.modulationOutputs[kLFOQuadPhaseOutput];
	
	// --- special unipolar from max output for tremolo
	//
	// --- first, convert to unipolar
	lfoOutputData.modulationOutputs[kUnipolarOutputFromMax] = bipolarToUnipolar(lfoOutputData.modulationOutputs[kLFONormalOutput]);
	lfoOutputData.modulationOutputs[kUnipolarOutputFromMin] = bipolarToUnipolar(lfoOutputData.modulationOutputs[kLFONormalOutput]);

	// --- then shift upwards by enough to put peaks right at 1.0
	//     NOTE: leaving the 0.5 in the equation - it is the unipolar offset when convering bipolar; but it could be changed...
	lfoOutputData.modulationOutputs[kUnipolarOutputFromMax] = lfoOutputData.modulationOutputs[kUnipolarOutputFromMax] + (1.0 - 0.5 - (parameters->outputAmplitude / 2.0));
	
	// --- then shift down enough to put troughs at 0.0
	lfoOutputData.modulationOutputs[kUnipolarOutputFromMin] = lfoOutputData.modulationOutputs[kUnipolarOutputFromMin] - (1.0 - 0.5 - (parameters->outputAmplitude / 2.0));

	// --- setup for next sample period
	advanceModulo(modCounter, phaseInc);

	if (rampMult < 1.0)
	{
		rampMult += rampInc;
	}
	
	// --- scale by amplituded
	return lfoOutputData;
}



