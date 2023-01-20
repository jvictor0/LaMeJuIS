#pragma once

namespace LogicMatrixConstants
{
    enum class ParamType : int
    {
        InputMuteSwitch = 0,
        MatrixSwitch = 1,
        EquationSwitch = 2,
        EquationOperatorKnob = 3,
        OutputKnob = 4,
        NumParamTypes = 5
    };

    static constexpr size_t x_numInputs = 5;
    static constexpr size_t x_numEquations = 6;
    static constexpr size_t x_numOutputs = 3;

    static constexpr size_t x_numParamsPerType[] =
    {
        x_numInputs /*InputMuteSwitch*/,
        x_numInputs * x_numEquations /*MatrixSwitch*/,
        x_numEquations /*EquationSwitch*/,
        x_numEquations /*EquationOperatorKnob*/,
        x_numOutputs /*OutputKnob*/
    };

    static constexpr size_t x_paramStartPerType[] = {
        0,
        x_numParamsPerType[0],
        x_numParamsPerType[0] + x_numParamsPerType[1],
        x_numParamsPerType[0] + x_numParamsPerType[1] + x_numParamsPerType[2],
        x_numParamsPerType[0] + x_numParamsPerType[1] + x_numParamsPerType[2] + x_numParamsPerType[3],
        x_numParamsPerType[0] + x_numParamsPerType[1] + x_numParamsPerType[2] + x_numParamsPerType[3] + x_numParamsPerType[4]
    };

    static constexpr size_t GetParamId(ParamType paramType, size_t paramId)
    {
        return x_paramStartPerType[static_cast<int>(paramType)] + paramId;
    }

    static constexpr size_t GetInputMuteSwitchId(size_t inputId)
    {
        return GetParamId(ParamType::InputMuteSwitch, inputId);
    }

    static constexpr size_t GetMatrixSwitchId(size_t inputId, size_t equationId)
    {
        return GetParamId(ParamType::MatrixSwitch, inputId + equationId * x_numInputs);
    }
    
    static constexpr size_t GetEquationSwitchId(size_t equationId)
    {
        return GetParamId(ParamType::EquationSwitch, equationId);
    }

    static constexpr size_t GetEquationOperatorKnobId(size_t equationId)
    {
        return GetParamId(ParamType::EquationOperatorKnob, equationId);
    }

    static constexpr size_t GetOutputKnobId(size_t outputId)
    {
        return GetParamId(ParamType::OutputKnob, outputId);
    }

    static constexpr size_t GetNumParams()
    {
        return x_paramStartPerType[5];
    }

    enum class InputType : int
    {
        MainInput,
        NumInputTypes,
    };

    static constexpr size_t x_numInputsPerType[] =
    {
        x_numInputs
    };

    static constexpr size_t x_inputStartPerType[] =
    {
        0,
        x_numInputsPerType[0]
    };

    static constexpr size_t GetInputId(InputType inputType, size_t inputId)
    {
        return x_inputStartPerType[static_cast<int>(inputType)] + inputId;
    }

    static constexpr size_t GetMainInputId(size_t inputId)
    {
        return GetInputId(InputType::MainInput, inputId);
    }

    static constexpr size_t GetNumInputs()
    {
        return x_inputStartPerType[static_cast<int>(InputType::NumInputTypes)];
    }

    enum class OutputType : int
    {
        MainOutput,
        TriggerOutput,
        NumOutputTypes
    };

    static constexpr size_t x_numOutputsPerType[] =
    {
        x_numOutputs,
        x_numOutputs
    };

    static constexpr size_t x_outputStartPerType[] =
    {
        0,
        x_numOutputsPerType[0],
        x_numOutputsPerType[0] + x_numOutputsPerType[1]
    };

    static constexpr size_t GetOutputId(OutputType outputType, size_t outputId)
    {
        return x_outputStartPerType[static_cast<int>(outputType)] + outputId;
    }

    static constexpr size_t GetMainOutputId(size_t outputId)
    {
        return GetOutputId(OutputType::MainOutput, outputId);
    }

    static constexpr size_t GetTriggerOutputId(size_t outputId)
    {
        return GetOutputId(OutputType::TriggerOutput, outputId);
    }

    static constexpr size_t GetNumOutputs()
    {
        return x_outputStartPerType[static_cast<int>(OutputType::NumOutputTypes)];
    }

    enum class LightType : int
    {
        InputLight,
        EquationLight,
        NumLightTypes,
    };

    static constexpr size_t x_numLightsPerType[] =
    {
        x_numInputs,
        x_numEquations
    };

    static constexpr size_t x_lightStartPerType[] =
    {
        0,
        x_numLightsPerType[0],
        x_numLightsPerType[0] + x_numLightsPerType[1]
    };

    static constexpr size_t GetLightId(LightType lightType, size_t lightId)
    {
        return x_lightStartPerType[static_cast<int>(lightType)] + lightId;
    }

    static constexpr size_t GetInputLightId(size_t lightId)
    {
        return GetLightId(LightType::InputLight, lightId);
    }

    static constexpr size_t GetEquationLightId(size_t lightId)
    {
        return GetLightId(LightType::EquationLight, lightId);
    }

    static constexpr size_t GetNumLights()
    {
        return x_lightStartPerType[static_cast<int>(LightType::NumLightTypes)];
    }
}
