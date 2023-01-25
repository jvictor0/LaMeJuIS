#include "LogicMatrix.hpp"

template<typename Enum>
Enum FloatToEnum(float in)
{
    return static_cast<Enum>(static_cast<int>(in + 0.5));
}

LogicMatrix::Input::SwitchVal
LogicMatrix::Input::GetSwitchVal()
{
    return FloatToEnum<SwitchVal>(m_switch->getValue());
}

void LogicMatrix::Input::SetValue(LogicMatrix::Input* prev)
{
    using namespace LogicMatrixConstants;
    bool oldValue = m_value;
    
    if (IsMuted())
    {
        m_value = MutedValue();
    }
    
    // If a cable is connected, use that value.
    //
    else if (m_port->isConnected())
    {
        float cableValue = m_port->getVoltage();
        m_schmittTrigger.process(cableValue);
        m_value = m_schmittTrigger.isHigh();
    }
    
    // Each input (except the first) is normaled to divide-by-two of the previous input.
    //
    else if (prev)
    {
        // If the previous input went low-to-high, flip this input.
        //
        if (prev->m_value && prev->m_changedThisFrame)
        {
            m_value = !m_value;
        }
    }
    
    m_changedThisFrame = m_value != oldValue;
    if (m_changedThisFrame)
    {
        m_light->setBrightness(m_value ? 1.f : 0.f);
    }
}

LogicMatrix::MatrixElement::SwitchVal
LogicMatrix::MatrixElement::GetSwitchVal()
{
    return FloatToEnum<SwitchVal>(m_switch->getValue());
}

bool LogicMatrix::MatrixElement::GetValue(Input* input, InputOverride ovr)
{
    bool normalValue = input->GetValue(ovr);
    switch (GetSwitchVal())
    {
        case SwitchVal::Normal: return normalValue;
        case SwitchVal::Inverted: return !normalValue;
        default: return false;
    }
}

LogicMatrix::LogicEquation::Operator
LogicMatrix::LogicEquation::GetOperator()
{
    return FloatToEnum<Operator>(m_operatorKnob->getValue());
}

LogicMatrix::LogicEquation::SwitchVal
LogicMatrix::LogicEquation::GetSwitchVal()
{
    return FloatToEnum<SwitchVal>(m_switch->getValue());
}

bool LogicMatrix::LogicEquation::GetValue(LogicMatrix::Input* inputArray, InputOverrides ovrs)
{
    using namespace LogicMatrixConstants;   

    size_t countHigh = 0;
    size_t countTotal = 0;
    for (size_t i = 0; i < x_numInputs; ++i)
    {       
        if (!m_elements[i].IsMuted())
        {
            ++countTotal;
            bool isHigh = m_elements[i].GetValue(&inputArray[i], ovrs.Get(i));
            if (isHigh)
            {
                ++countHigh;
            }
        }
    }

    bool ret = false;
    switch (GetOperator())
    {
        case Operator::Or: ret = (countHigh > 0); break;
        case Operator::And: ret = (countHigh == countTotal); break;
        case Operator::Xor: ret = (countHigh % 2 == 1); break;
        case Operator::AtLeastTwo: ret = (countHigh >= 2); break;
        case Operator::Majority: ret = (2 * countHigh > countTotal); break;
    }

    if (!ovrs.HasOverride())
    {
        m_light->setBrightness(ret ? 1.f : 0.f);
    }
    
    return ret;
}

LogicMatrix::MatrixEvalResult LogicMatrix::EvalMatrix(InputOverrides ovrs)
{
    using namespace LogicMatrixConstants;
    MatrixEvalResult result;

    for (size_t i = 0; i < x_numEquations; ++i)
    {
        size_t outputId = m_equations[i].GetOutputTarget();
        ++result.m_total[outputId];
        bool isHigh = m_equations[i].GetValue(m_inputs, ovrs);
        if (isHigh)
        {
            ++result.m_high[outputId];
        }
    }

    return result;
}

constexpr float LogicMatrix::Output::x_voltages[];

LogicMatrix::Output::Interval
LogicMatrix::Output::GetInterval()
{
    return FloatToEnum<Interval>(m_intervalKnob->getValue());
}

float LogicMatrix::Output::GetValue(uint8_t high, uint8_t total)
{
    Interval interval = GetInterval();
    switch (interval)
    {
        case Interval::Or: return high > 0 ? 5.f : 0.f;
        case Interval::And: return high == total ? 5.f : 0.f;
        case Interval::Avg: return static_cast<float>(high) * 5.0 / static_cast<float>(total);
        default: return x_voltages[static_cast<size_t>(interval)] * high;
    }
}

LogicMatrix::Mode
LogicMatrix::GetMode()
{
    using namespace LogicMatrixConstants;      
    return FloatToEnum<Mode>(params[GetModeKnobId()].getValue());
}

LogicMatrix::LogicMatrix()
{
    using namespace LogicMatrixConstants;   
    
    config(GetNumParams(), GetNumInputs(), GetNumOutputs(), GetNumLights());
    for (size_t i = 0; i < x_numInputs; ++i)
    {
        configParam(GetInputMuteSwitchId(i), 0.f, 2.f, 1.f, "");
        configInput(GetMainInputId(i), "");
        
        for (size_t j = 0; j < x_numEquations; ++j)
        {
            configParam(GetMatrixSwitchId(i, j), 0.f, 2.f, 1.f, "");
            m_equations[j].m_elements[i].Init(&params[GetMatrixSwitchId(i, j)]);
        }

        m_inputs[i].Init(
            &inputs[GetMainInputId(i)],
            &lights[GetInputLightId(i)],
            &params[GetInputMuteSwitchId(i)]);
    }
    
    for (size_t i = 0; i < x_numEquations; ++i)
    {
        configParam(GetEquationSwitchId(i), 0.f, 2.f, 1.f, "");
        configParam(GetEquationOperatorKnobId(i), 0.f, 4.f, 0.f, "");

        m_equations[i].Init(
            &params[GetEquationSwitchId(i)],
            &params[GetEquationOperatorKnobId(i)],
            &lights[GetEquationLightId(i)]);
    }
    
    for (size_t i = 0; i < x_numOutputs; ++i)
    {
        configParam(GetOutputKnobId(i), 0.f, 9.f, 0.f, "");
        configOutput(GetMainOutputId(i), "");
        configOutput(GetTriggerOutputId(i), "");

        m_outputs[i].Init(
            &params[GetOutputKnobId(i)],
            &outputs[GetMainOutputId(i)],
            &outputs[GetTriggerOutputId(i)]);
    }
    
    configParam(GetModeKnobId(), 0.f, 4.f, 0.f, "");
}

void LogicMatrix::ProcessInputs()
{
    using namespace LogicMatrixConstants;
    for (size_t i = 0; i < x_numInputs; ++i)
    {
        m_inputs[i].SetValue(i > 0 ? &m_inputs[i - 1] : nullptr);
    }
}

void LogicMatrix::ProcessOutputs(float dt)
{
    using namespace LogicMatrixConstants;

    MatrixEvalResult evalResult = EvalMatrix(InputOverrides());

    Mode mode = GetMode();
    switch(mode)
    {
        case Mode::Direct:
        {
            for (size_t i = 0; i < x_numOutputs; ++i)
            {
                uint8_t high = 0;
                uint8_t total = 0;

                // First evaluate the equation directly going to the trigger output.
                //
                bool trigVal = m_equations[2 * i].GetValueDirect(m_inputs, &high, &total);
                m_equations[2 * i + 1].GetValueDirect(m_inputs, &high, &total);
                m_outputs[i].SetValueDirect(trigVal, m_outputs[i].GetValue(high, total));
            }

            break;
        }

        case Mode::Independent:
        {
            for (size_t i = 0; i < x_numOutputs; ++i)
            {
                m_outputs[i].SetValue(m_outputs[i].GetValue(evalResult.m_high[i], evalResult.m_total[i]), dt);
            }

            break;
        }

        case Mode::OneVoice:
        {
            // In one voice mode, the middle output will be set to the sum.  Set the other two to the average, for cool
            // stepped voltages.
                //
            m_outputs[0].SetValue(5.0 * static_cast<float>(evalResult.m_high[0]) / evalResult.m_total[0], dt);
            m_outputs[1].SetValue(ComputePitch(evalResult), dt);
            m_outputs[2].SetValue(5.0 * static_cast<float>(evalResult.m_high[2]) / evalResult.m_total[2], dt);
            break;
        }

        case Mode::ThreeVoice:
        {
            float leadPitch = EvalMatrixAndComputePitch(InputOverrides());
            float bassPitch = 10.f;

            // Set the bass equal to the lowest implicit chord tone.
            //
            for (InputOverride ovr0 : {InputOverride::OverrideLow, InputOverride::OverrideHigh})
            {
                for (InputOverride ovr1 : {InputOverride::OverrideLow, InputOverride::OverrideHigh})
                {
                    bassPitch = std::min<float>(bassPitch, EvalMatrixAndComputePitch(InputOverrides(ovr0, ovr1)));
                }
            }

            // For the third voice, set it to the lower of the two, so long as that isn't the bass value (unless they both are).
            // The bass value will of course be no larger than either of these.
            //
            float zeroOvrLow = EvalMatrixAndComputePitch(InputOverrides(InputOverride::OverrideLow, InputOverride::DontOverride));
            float zeroOvrHigh = EvalMatrixAndComputePitch(InputOverrides(InputOverride::OverrideHigh, InputOverride::DontOverride));
            float tenorPitch = zeroOvrLow == bassPitch ? zeroOvrHigh : std::min<float>(zeroOvrHigh, zeroOvrLow);

            m_outputs[0].SetValue(bassPitch, dt);
            m_outputs[1].SetValue(leadPitch, dt);
            m_outputs[2].SetValue(tenorPitch, dt);
            break;
        }

        case Mode::Chord:
        {
            float pitches[4];
            size_t ix = 0;
            for (InputOverride ovr0 : {InputOverride::OverrideLow, InputOverride::OverrideHigh})
            {
                for (InputOverride ovr1 : {InputOverride::OverrideLow, InputOverride::OverrideHigh})
                {
                    pitches[ix] = EvalMatrixAndComputePitch(InputOverrides(ovr0, ovr1));
                    ++ix;
                }
            }

            static const size_t x_numPitches = 4;
            std::sort(pitches, pitches + x_numPitches);
            bool usedPitches[] = {false, false, false, false};
            m_outputs[0].SetValue(pitches[0], dt);
            usedPitches[0] = true;
            for (size_t i = 1; i < x_numOutputs; ++i)
            {
                size_t ixToUse = x_numPitches;
                for (size_t j = 0; j < x_numPitches; ++j)
                {
                    if (!usedPitches[j] &&
                        (ixToUse == x_numPitches ||
                         std::abs(m_outputs[i].m_value - pitches[j]) <
                         std::abs(m_outputs[i].m_value - pitches[ixToUse])))
                    {
                        ixToUse = j;                        
                    }
                }

                usedPitches[ixToUse] = true;
                m_outputs[i].SetValue(pitches[ixToUse], dt);
            }

            break;
        }
    }    
}

void LogicMatrix::process(const ProcessArgs& args)
{
    ProcessInputs();
    ProcessOutputs(args.sampleTime);
}
