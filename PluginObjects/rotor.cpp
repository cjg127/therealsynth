#include "rotor.h"

bool Rotor::update(bool updateAllModRoutings)
{
	if (!updateAllModRoutings)
		return true;
	parameters->xLFOParameters->frequency_Hz = parameters->frequency;
	parameters->xLFOParameters->outputAmplitude = parameters->xAmp;
	parameters->yLFOParameters->frequency_Hz = parameters->frequency;
	parameters->yLFOParameters->outputAmplitude = parameters->yAmp;

	xLFO->update(updateAllModRoutings);
	yLFO->update(updateAllModRoutings);
	return true;
}

const ModOutputData Rotor::renderModulatorOutput()
{
	ModOutputData rotor_output;

	xLFOOutput = xLFO->renderModulatorOutput();
	double x_render = xLFOOutput.modulationOutputs[kLFONormalOutput];
	rotor_output.modulationOutputs[kRotorXOutput] = x_render;

	yLFOOutput = yLFO->renderModulatorOutput();
	double y_render = yLFOOutput.modulationOutputs[kLFOQuadPhaseOutput];
	rotor_output.modulationOutputs[kRotorYOutput] = y_render;

	return rotor_output;
}