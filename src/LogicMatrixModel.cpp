#include "LogicMatrix.hpp"

struct LogicMatrixWidget : ModuleWidget
{
    static constexpr float x_hp = 5.08;

    static constexpr float x_jackLightOffsetHP = 1.0;
    static constexpr float x_jackStartYHP = 4.25;
    static constexpr float x_jackSpacingHP = 3.0;

    static constexpr float x_firstMatrixSwitchXHP = 15.0;
    static constexpr float x_firstMatrixSwitchYHP = 3.5;
    static constexpr float x_switchSpacingXHP = 1.5;
    static constexpr float x_rowYSpacing = 3.5;

    static constexpr float x_operationKnobXHP = 25.25;
    static constexpr float x_firstOperatorKnobYHP = 3.25;

    static constexpr float x_operationSwitchXHP = 27.5;

    static constexpr float x_firstKnobMatrixXHP = 31.75;
    static constexpr float x_firstKnobMatrixYHP = 3.50;
    static constexpr float x_knobMatrixSpacingXHP = 4.5;
    static constexpr float x_knobMatrixSpacingYHP = 4.0;

    static constexpr float x_firstCoMuteXHP = 31.0;
    static constexpr float x_firstCoMuteYHP = 16.0;

    Vec GetInputJackMM(size_t inputId)
    {
        return Vec(x_hp,
                   x_hp * (x_jackStartYHP + inputId * x_jackSpacingHP));
    }

    Vec JackToLight(Vec jackPos)
    {
        return jackPos.plus(Vec(x_hp * x_jackLightOffsetHP, - x_hp * x_jackLightOffsetHP));
    }

    Vec GetOperationOutputJackMM(size_t operationId)
    {
        return GetInputJackMM(operationId).plus(Vec(x_hp * x_jackSpacingHP, 0));
    }

    Vec GetMainOutputJackMM(size_t accumulatorId)
    {
        return GetInputJackMM(2 * accumulatorId).plus(Vec(2 * x_hp * x_jackSpacingHP, 0));
    }

    Vec GetTriggerOutputJackMM(size_t accumulatorId)
    {
        return GetInputJackMM(2 * accumulatorId + 1).plus(Vec(2 * x_hp * x_jackSpacingHP, 0));
    }

    Vec GetCVOutputJackMM(size_t accumulatorId)
    {
        return GetInputJackMM(accumulatorId).plus(Vec(3 * x_hp * x_jackSpacingHP, 0));
    }

    Vec GetIntervalInputJackMM(size_t accumulatorId)
    {
        return GetInputJackMM(accumulatorId + 3).plus(Vec(3 * x_hp * x_jackSpacingHP, 0));
    }
    
    Vec GetMatrixSwitchMM(size_t inputId, size_t operationId)
    {
        return Vec(x_hp * (x_firstMatrixSwitchXHP + inputId * x_switchSpacingXHP),
                   x_hp * (x_firstMatrixSwitchYHP + operationId * x_rowYSpacing));
    }

    Vec GetOperatorKnobMM(size_t operationId)
    {
        return Vec(x_hp * x_operationKnobXHP,
                   x_hp * (x_firstOperatorKnobYHP + operationId * x_rowYSpacing));
    }

    Vec GetOperationSwitchMM(size_t operationId)
    {
        return Vec(x_hp * x_operationSwitchXHP,
                   x_hp * (x_firstMatrixSwitchYHP + operationId * x_rowYSpacing));
    }

    Vec GetKnobMatrixMM(size_t knobColumn, size_t accumulatorId)
    {
        return Vec(x_hp * (x_firstKnobMatrixXHP + knobColumn * x_knobMatrixSpacingXHP),
                   x_hp * (x_firstKnobMatrixYHP + accumulatorId * x_knobMatrixSpacingYHP));
    }

    Vec GetCoMuteSwitchMM(size_t inputId, size_t accumulatorId)
    {
        return Vec(x_hp * (x_firstCoMuteXHP + inputId * x_switchSpacingXHP),
                   x_hp * (x_firstCoMuteYHP + accumulatorId * x_rowYSpacing));
    }

	LogicMatrixWidget(LogicMatrix* module)
    {
        using namespace LogicMatrixConstants;   

		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/LogicMatrix.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        for (size_t i = 0; i < x_numInputs; ++i)
        {
            addInput(createInputCentered<PJ301MPort>(
                         mm2px(GetInputJackMM(i)),
                         module,
                         GetMainInputId(i)));

             addChild(createLightCentered<MediumLight<RedLight>>(
                          mm2px(JackToLight(GetInputJackMM(i))),
                          module,
                          GetInputLightId(i)));

            for (size_t j = 0; j < x_numOperations; ++j)
            {
                addParam(createParamCentered<NKK>(
                             mm2px(GetMatrixSwitchMM(i, j)),
                             module,
                             GetMatrixSwitchId(i, j)));
            }

            for (size_t j = 0; j < x_numAccumulators; ++j)
            {
                addParam(createParamCentered<NKK>(
                             mm2px(GetCoMuteSwitchMM(i, j)),
                             module,
                             GetPitchCoMuteSwitchId(i, j)));                
            }
        }

        for (size_t i = 0; i < x_numOperations; ++i)
        {
            addParam(createParamCentered<RoundBlackSnapKnob>(
                         mm2px(GetOperatorKnobMM(i)),
                         module,
                         GetOperatorKnobId(i)));
            addParam(createParamCentered<NKK>(
                         mm2px(GetOperationSwitchMM(i)),
                         module,
                         GetOperationSwitchId(i)));
            addOutput(createOutputCentered<PJ301MPort>(
                          mm2px(GetOperationOutputJackMM(i)),
                          module,
                          GetOperationOutputId(i)));            
            addChild(createLightCentered<MediumLight<RedLight>>(
                         mm2px(JackToLight(GetOperationOutputJackMM(i))),
                         module,
                         GetOperationLightId(i)));
        }

        for (size_t i = 0; i < x_numAccumulators; ++i)
        {
            addOutput(createOutputCentered<PJ301MPort>(
                          mm2px(GetMainOutputJackMM(i)),
                          module,
                          GetMainOutputId(i)));
            addOutput(createOutputCentered<PJ301MPort>(
                          mm2px(GetTriggerOutputJackMM(i)),
                          module,
                          GetTriggerOutputId(i)));
            addChild(createLightCentered<MediumLight<RedLight>>(
                         mm2px(JackToLight(GetTriggerOutputJackMM(i))),
                         module,
                         GetTriggerLightId(i)));
            addOutput(createOutputCentered<PJ301MPort>(
                          mm2px(GetCVOutputJackMM(i)),
                          module,
                          GetCVOutputId(i)));
            addChild(createLightCentered<MediumLight<RedLight>>(
                         mm2px(JackToLight(GetCVOutputJackMM(i))),
                         module,
                         GetCVLightId(i)));
            addInput(createInputCentered<PJ301MPort>(
                         mm2px(GetIntervalInputJackMM(i)),
                         module,
                         GetIntervalCVInputId(i)));
            
            addParam(createParamCentered<RoundBlackSnapKnob>(
                         mm2px(GetKnobMatrixMM(0, i)),
                         module,
                         GetAccumulatorIntervalKnobId(i)));
            addParam(createParamCentered<RoundBlackKnob>(
                         mm2px(GetKnobMatrixMM(1, i)),
                         module,
                         GetPitchPercentileKnobId(i)));

        }
	}
};

Model* modelLogicMatrix = createModel<LogicMatrix, LogicMatrixWidget>("LogicMatrix");
