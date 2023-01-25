#include "plugin.hpp"
#include <cstddef>
#include "LogicMatrixConstants.hpp"

struct LogicMatrix : Module
{
    enum class Mode : char
    {
        Direct = 0,
        Independent = 1,
        OneVoice = 2,
        ThreeVoice = 3,
        Chord = 4
    };

    enum class InputOverride : char
    {
        DontOverride,
        OverrideHigh,
        OverrideLow
    };

    static bool ApplyOverride(InputOverride ovr, bool value)
    {
        bool ret = false;
        switch (ovr)
        {
            case InputOverride::DontOverride: return value;
            case InputOverride::OverrideHigh: ret = true; break;
            case InputOverride::OverrideLow: ret = false; break;
        }

        return ret;
    }
    
    struct Input
    {
        enum class SwitchVal : char
        {
            MuteDown = 0, 
            Unmuted = 1,
            MuteUp = 2,
        };
        
        rack::engine::Input* m_port = nullptr;
        rack::dsp::TSchmittTrigger<float> m_schmittTrigger;
        rack::engine::Light* m_light = nullptr;
        rack::engine::Param* m_switch = nullptr;
        bool m_value = false;
        bool m_changedThisFrame = false;

        void Init(
            rack::engine::Input* port,
            rack::engine::Light* light,
            rack::engine::Param* swtch)
        {
            m_port = port;
            m_light = light;
            m_switch = swtch;
        }
        
        SwitchVal GetSwitchVal();
        
        bool IsMuted()
        {
            return GetSwitchVal() != SwitchVal::Unmuted;
        }
        
        bool MutedValue()
        {
            return GetSwitchVal() == SwitchVal::MuteUp;
        }
        
        void SetValue(Input* prev);

        bool GetValue(InputOverride ovr)
        {
            return ApplyOverride(ovr, m_value);
        }
    };

    struct InputOverrides
    {
        InputOverrides()
            : m_ovr0(InputOverride::DontOverride)
            , m_ovr1(InputOverride::DontOverride)
        {
        }

        InputOverrides(
            InputOverride ovr0,
            InputOverride ovr1)
            : m_ovr0(ovr0)
            , m_ovr1(ovr1)
        {
        }

        InputOverride Get(size_t i)
        {
            switch (i)
            {
                case 0: return m_ovr0;
                case 1: return m_ovr1;
                default: return InputOverride::DontOverride;
            }
        }

        bool HasOverride()
        {
            return m_ovr0 == InputOverride::DontOverride &&
                m_ovr1 == InputOverride::DontOverride;
        }
        
        InputOverride m_ovr0;
        InputOverride m_ovr1;
    };

    struct MatrixElement
    {
        enum class SwitchVal : char
        {
            Inverted = 0,
            Muted = 1,
            Normal = 2
        };
        
        rack::engine::Param* m_switch = nullptr;

        void Init(rack::engine::Param* swtch)
        {
            m_switch = swtch;
        }
        
        SwitchVal GetSwitchVal();
        
        bool IsMuted()
        {
            return GetSwitchVal() == SwitchVal::Muted;
        }
        
        bool GetValue(Input* input, InputOverride ovr);
    };

    struct LogicEquation
    {
        enum class Operator : char
        {
            Or = 0,
            And = 1,
            Xor = 2,
            AtLeastTwo = 3,
            Majority = 4
        };
        
        enum class SwitchVal : char
        {
            Down = 0,
            Middle = 1,
            Up = 2
        };

        Operator GetOperator();
        SwitchVal GetSwitchVal();
        
        rack::engine::Param* m_switch = nullptr;
        rack::engine::Param* m_operatorKnob = nullptr;
        rack::engine::Light* m_light = nullptr;

        void Init(
            rack::engine::Param* swtch,
            rack::engine::Param* operatorKnob,
            rack::engine::Light* light)
        {
            m_switch = swtch;
            m_operatorKnob = operatorKnob;
            m_light = light;
        }            

        bool GetValue(Input* inputArray, InputOverrides ovrs);
        bool GetValueDirect(Input* inputArray, uint8_t* high, uint8_t* total)
        {
            bool value = GetValue(inputArray, InputOverrides());
            LogicEquation::SwitchVal swtch = GetSwitchVal();
            if (swtch != LogicEquation::SwitchVal::Middle)
            {
                ++(*total);
                if (value == (swtch == LogicEquation::SwitchVal::Up))
                {
                    ++(*high);
                }
            }

            return value;
        }

        // Up is output zero but input id 2, so invert.
        //
        size_t GetOutputTarget()
        {
            using namespace LogicMatrixConstants;
            return x_numOutputs - static_cast<size_t>(GetSwitchVal()) - 1;
        }
        
        LogicMatrix::MatrixElement m_elements[LogicMatrixConstants::x_numInputs];
    };

    struct MatrixEvalResult
    {
        MatrixEvalResult()
        {
            using namespace LogicMatrixConstants;
            for (size_t i = 0; i < x_numOutputs; ++i)
            {
                m_high[i] = 0;
                m_total[i] = 0;
            }
        }
        
        uint8_t m_high[LogicMatrixConstants::x_numOutputs];
        uint8_t m_total[LogicMatrixConstants::x_numOutputs];
    };

    MatrixEvalResult EvalMatrix(InputOverrides ovrs);

    float ComputePitch(MatrixEvalResult evalResult)
    {
        using namespace LogicMatrixConstants;
        float result = 0;
        for (size_t i = 0; i < x_numOutputs; ++i)
        {
            if (m_outputs[i].IsPitch())
            {
                result += m_outputs[i].GetValue(evalResult.m_high[i], evalResult.m_total[i]);
            }
        }

        return result;
    }

    float EvalMatrixAndComputePitch(InputOverrides ovrs)
    {
        return ComputePitch(EvalMatrix(ovrs));
    }
    
    struct Output
    {
        enum class Interval
        {
            Or = 0,
            And = 1,
            Avg = 2,
            WholeStep = 3,
            MinorThird = 4,
            MajorThird = 5,
            PerfectFourth = 6,
            PerfectFifth = 7,
            MinorSeventh = 8,
            Octave = 9
        };

        static constexpr float x_voltages[] = {
            0 /*And*/,
            0 /*Or*/,
            0 /*Avg*/,
            0.16992500144231237/*whole tone = log_2(9/8)*/,
            0.2630344058337938 /*minor third = log_2(6/5)*/,
            0.32192809488736235 /*major third = log_2(5/4)*/,
            0.4150374992788437 /*perfect fourth = log_2(4/3)*/,
            0.5849625007211562 /*pefect fifth = log_2(3/2)*/,
            0.8073549220576041 /*minor seventh = log_2(7/4)*/,
            1.0 /*octave = log_2(2)*/
        };

        rack::engine::Param* m_intervalKnob = nullptr;
        rack::engine::Output* m_mainOut = nullptr;
        rack::engine::Output* m_triggerOut = nullptr;
        rack::dsp::PulseGenerator m_pulseGen;
        float m_value = 0.0;

        Interval GetInterval();

        bool IsPitch()
        {
            Interval interval = GetInterval();
            switch(interval)
            {
                case Interval::And:
                case Interval::Or:
                case Interval::Avg:
                    return false;
                default:
                    return true;
            }
        }
        
        float GetValue(uint8_t high, uint8_t total);

        void SetValue(float value, float dt)
        {
            bool changedThisFrame = (value != m_value);
            m_value = value;
            m_mainOut->setVoltage(value);

            if (changedThisFrame)
            {
                m_pulseGen.trigger();
            }

            m_triggerOut->setVoltage(m_pulseGen.process(dt) ? 5.f : 0.f);
        }

        void SetValueDirect(bool trigVal, float value)
        {
            // There is no trigger in direct mode, so set m_changedThisFrame to false
            //
            m_value = value;
            m_triggerOut->setVoltage(trigVal ? 5.f : 0.f);
            m_mainOut->setVoltage(value);
        }
        
        void Init(
            rack::engine::Param* intervalKnob,
            rack::engine::Output* mainOut,
            rack::engine::Output* triggerOut)
        {
            m_intervalKnob = intervalKnob;
            m_mainOut = mainOut;
            m_triggerOut = triggerOut;
        }
    };

    Mode GetMode();

    void ProcessInputs();
    void ProcessOutputs(float dt);
    
	LogicMatrix();

    ~LogicMatrix()
    {
    }

    void process(const ProcessArgs& args) override;

    Input m_inputs[LogicMatrixConstants::x_numInputs];
    LogicEquation m_equations[LogicMatrixConstants::x_numEquations];
    Output m_outputs[LogicMatrixConstants::x_numOutputs];
};
