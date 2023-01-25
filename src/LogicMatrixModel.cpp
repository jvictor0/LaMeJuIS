#include "LogicMatrix.hpp"

struct LogicMatrixWidget : ModuleWidget
{
    static constexpr float x_hp = 5.08;

    static constexpr float x_inputJackHP = 1.0;
    static constexpr float x_inputJackStartYMM = 21.82;
    static constexpr float x_inputJackDeltaYMM = 20.32;
    static constexpr float x_inputLightDeltaYMM = 10.16;

    static constexpr float x_inputMuteSwitchXHP = 2.5;
    
    static constexpr float x_matrixSwitchStartXHP = 4.5;
    static constexpr float x_matrixSwitchDeltaXHP = 1.0;
    static constexpr float x_matrixSwitchStartYMM = 21.82;
    static constexpr float x_matrixSwitchDeltaYMM = 20.465;

    static constexpr float x_equationOperatorKnobXHP = 11.5;
    static constexpr float x_equationSwitchXHP = 14;

    static constexpr float x_outputKnobXHP = 18;
    static constexpr float x_outputJackStartYMM = 21.82;
    static constexpr float x_outputJackDeltaYMM = 40.64;
    static constexpr float x_outputJackXHP = 19;

    Vec GetInputJackMM(size_t inputId)
    {
        return Vec(x_hp * x_inputJackHP,
                   x_inputJackStartYMM + inputId * x_inputJackDeltaYMM);
    }

    Vec GetInputLightMM(size_t inputId)
    {
        return Vec(x_hp * x_inputJackHP,
                   x_inputJackStartYMM + inputId * x_inputJackDeltaYMM + x_inputLightDeltaYMM);
    }

    Vec GetInputMuteSwitchMM(size_t inputId)
    {
        return Vec(x_hp * x_inputMuteSwitchXHP,
                   x_inputJackStartYMM + inputId * x_inputJackDeltaYMM + x_inputLightDeltaYMM);
    }
    
    Vec GetMatrixSwitchMM(size_t inputId, size_t equationId)
    {
        return Vec(x_hp * (x_matrixSwitchStartXHP + inputId * x_matrixSwitchDeltaXHP),
                   x_matrixSwitchStartYMM + equationId * x_matrixSwitchDeltaYMM);
    }

    Vec GetEquationOperatorKnobMM(size_t equationId)
    {
        return Vec(x_hp * x_equationOperatorKnobXHP,
                   x_matrixSwitchStartYMM + equationId * x_matrixSwitchDeltaYMM);
    }

    Vec GetEquationOperatorSwitchMM(size_t equationId)
    {
        return Vec(x_hp * x_equationSwitchXHP,
                   x_matrixSwitchStartYMM + equationId * x_matrixSwitchDeltaYMM);
    }

    // Put the equation light 2hp above the equation switch?
    //
    Vec GetEquationLightMM(size_t equationId)
    {
        return Vec(x_hp * x_equationSwitchXHP,
                   x_matrixSwitchStartYMM + equationId * x_matrixSwitchDeltaYMM - 2 * x_hp);        
    }

    Vec GetMainOutputJackMM(size_t outputId)
    {
        return Vec(x_hp * x_outputJackXHP,
                   x_outputJackStartYMM + outputId * x_outputJackDeltaYMM);
    }

    Vec GetTriggerOutputJackMM(size_t outputId)
    {
        return Vec(x_hp * x_outputJackXHP,
                   x_outputJackStartYMM + 2 * x_hp + outputId * x_outputJackDeltaYMM);
    }

    Vec GetOutputKnobMM(size_t outputId)
    {
        return Vec(x_hp * x_outputJackXHP,
                   x_outputJackStartYMM + 5 * x_hp + outputId * x_outputJackDeltaYMM);
    }

    Vec GetModeKnobMM()
    {
        return Vec(x_hp * (x_inputJackHP + 1),
                   x_inputJackStartYMM - 3 * x_hp);
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
                          mm2px(GetInputLightMM(i)),
                          module, GetInputLightId(i)));

            addParam(createParamCentered<NKK>(
                         mm2px(GetInputMuteSwitchMM(i)),
                         module,
                         GetInputMuteSwitchId(i)));
            
            for (size_t j = 0; j < x_numEquations; ++j)
            {
                addParam(createParamCentered<NKK>(
                             mm2px(GetMatrixSwitchMM(i, j)),
                             module,
                             GetMatrixSwitchId(i, j)));
            }
        }

        for (size_t i = 0; i < x_numEquations; ++i)
        {
            addParam(createParamCentered<RoundBlackSnapKnob>(
                         mm2px(GetEquationOperatorKnobMM(i)),
                         module,
                         GetEquationOperatorKnobId(i)));
            addParam(createParamCentered<NKK>(
                         mm2px(GetEquationOperatorSwitchMM(i)),
                         module,
                         GetEquationSwitchId(i)));
            addChild(createLightCentered<MediumLight<RedLight>>(
                         mm2px(GetEquationLightMM(i)),
                         module, GetEquationLightId(i)));
        }

        for (size_t i = 0; i < x_numOutputs; ++i)
        {
            addParam(createParamCentered<RoundBlackSnapKnob>(
                         mm2px(GetOutputKnobMM(i)),
                         module,
                         GetOutputKnobId(i)));
            addOutput(createOutputCentered<PJ301MPort>(
                          mm2px(GetMainOutputJackMM(i)),
                          module,
                          GetMainOutputId(i)));
            addOutput(createOutputCentered<PJ301MPort>(
                          mm2px(GetTriggerOutputJackMM(i)),
                          module,
                          GetTriggerOutputId(i)));
        }

        addParam(createParamCentered<RoundBlackSnapKnob>(
                     mm2px(GetModeKnobMM()),
                     module,
                     GetModeKnobId()));
	}
};


Model* modelLogicMatrix = createModel<LogicMatrix, LogicMatrixWidget>("LogicMatrix");
