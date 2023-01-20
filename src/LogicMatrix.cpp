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

bool LogicMatrix::MatrixElement::GetValue(Input* input, InputOverride ovr, bool* overrideHappened)
{
    bool normalValue = input->GetValue(ovr, overrideHappened);
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
    bool overrideHappened = false;
    for (size_t i = 0; i < x_numInputs; ++i)
    {
        if (!m_elements[i].IsMuted())
        {
            ++countTotal;
            bool isHigh = m_elements[i].GetValue(&inputArray[i], ovrs.Get(i), &overrideHappened);
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

    if (!overrideHappened)
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
}

void LogicMatrix::ProcessInputs()
{
    using namespace LogicMatrixConstants;
    for (size_t i = 0; i < x_numInputs; ++i)
    {
        m_inputs[i].SetValue(i > 0 ? &m_inputs[i - 1] : nullptr);
    }
}

void LogicMatrix::ProcessOutputs()
{
    using namespace LogicMatrixConstants;
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
        }
        
        default:
        {
        }
    }    
}

void LogicMatrix::process(const ProcessArgs& args)
{
    ProcessInputs();
    ProcessOutputs();
}
