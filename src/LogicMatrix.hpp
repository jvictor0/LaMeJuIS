#include "plugin.hpp"
#include <cstddef>
#include "LogicMatrixConstants.hpp"

struct LogicMatrix : Module
{
    struct Input
    {
        rack::engine::Input* m_port = nullptr;
        rack::dsp::TSchmittTrigger<float> m_schmittTrigger;
        rack::engine::Light* m_light = nullptr;
        bool m_value = false;
        uint8_t m_counter = 0;

        void Init(
            rack::engine::Input* port,
            rack::engine::Light* light)
        {
            m_port = port;
            m_light = light;
            m_counter = 0;
        }
        
        void SetValue(Input* prev);
    };

    struct MatrixElement
    {
        enum class SwitchVal : char
        {
            Inverted = 0,
            Muted = 1,
            Normal = 2
        };
        
        void Init(rack::engine::Param* swtch)
        {
            m_switch = swtch;
        }
        
        SwitchVal GetSwitchVal();
        
        rack::engine::Param* m_switch = nullptr;
    };

    struct InputVector
    {
        InputVector()
            : m_bits(0)
        {
        }

        InputVector(uint8_t bits)
            : m_bits(bits)
        {
        }
        
        bool Get(size_t i)
        {
            return (m_bits & (1 << i)) >> i;
        }

        void Set(size_t i, bool value)
        {
            if (value)
            {
                m_bits |= 1 << i;
            }
            else
            {
                m_bits &= ~(1 << i);
            }
        }

        size_t CountSetBits();

        uint8_t m_bits;
    };
    
    struct LogicOperation
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
        
        void SetBitVectors()
        {
            using namespace LogicMatrixConstants;
            for (size_t i = 0; i < x_numInputs; ++i)
            {
                MatrixElement::SwitchVal switchVal = m_elements[i].GetSwitchVal();
                m_active.Set(i, switchVal != MatrixElement::SwitchVal::Muted);
                m_inverted.Set(i, switchVal == MatrixElement::SwitchVal::Inverted);
            }
        }
        
        void Init(
            rack::engine::Param* swtch,
            rack::engine::Param* operatorKnob,
            rack::engine::Output* output,
            rack::engine::Light* light)
        {
            m_switch = swtch;
            m_operatorKnob = operatorKnob;
            m_output = output;
            m_light = light;
        }            

        bool GetValue(InputVector inputVector);

        void SetOutput(bool value)
        {
            m_output->setVoltage(value ? 5.f : 0.f);
            m_light->setBrightness(value ? 1.f : 0.f);
        }

        // Up is output zero but input id 2, so invert.
        //
        size_t GetOutputTarget()
        {
            using namespace LogicMatrixConstants;
            return x_numAccumulators - static_cast<size_t>(GetSwitchVal()) - 1;
        }

        rack::engine::Param* m_switch = nullptr;
        rack::engine::Param* m_operatorKnob = nullptr;
        rack::engine::Light* m_light = nullptr;
        rack::engine::Output* m_output = nullptr;
        InputVector m_active;
        InputVector m_inverted;

        LogicMatrix::MatrixElement m_elements[LogicMatrixConstants::x_numInputs];
    };

    struct Accumulator
    {
        enum class Interval
        {
            Off = 0,
            HalfStep = 1,
            WholeStep = 2,
            MinorThird = 3,
            MajorThird = 4,
            PerfectFourth = 5,
            PerfectFifth = 6,
            MinorSeventh = 7,
            Octave = 8
        };

        static constexpr float x_voltages[] = {
            0 /*Off*/,
            0.09310940439 /*half step = log_2(16/15)*/,
            0.16992500144231237/*whole tone = log_2(9/8)*/,
            0.2630344058337938 /*minor third = log_2(6/5)*/,
            0.32192809488736235 /*major third = log_2(5/4)*/,
            0.4150374992788437 /*perfect fourth = log_2(4/3)*/,
            0.5849625007211562 /*pefect fifth = log_2(3/2)*/,
            0.8073549220576041 /*minor seventh = log_2(7/4)*/,
            1.0 /*octave = log_2(2)*/
        };

        rack::engine::Param* m_intervalKnob = nullptr;
        rack::engine::Output* m_cvOutput = nullptr;
        rack::engine::Light* m_cvOutLight = nullptr;

        Interval GetInterval();
        
        float GetPitch();

        void Init(
            rack::engine::Param* intervalKnob,
            rack::engine::Output* cvOutput,
            rack::engine::Light* cvOutLight)
        {
            m_intervalKnob = intervalKnob;
            m_cvOutput = cvOutput;
            m_cvOutLight = cvOutLight;
        }
    };

    struct MatrixEvalResult
    {
        MatrixEvalResult()
        {
            using namespace LogicMatrixConstants;
            
            for (size_t i = 0; i < x_numAccumulators; ++i)
            {
                m_high[i] = 0;
                m_total[i] = 0;
            }
        }

        void SetPitch(Accumulator* accumulators)
        {
            using namespace LogicMatrixConstants;
            
            float result = 0;
            for (size_t i = 0; i < x_numAccumulators; ++i)
            {
                result += accumulators[i].GetPitch() * m_high[i];
            }

            m_pitch = result;
        }

        bool operator<(const MatrixEvalResult& other) const
        {
            return m_pitch < other.m_pitch;
        }
        
        uint8_t m_high[LogicMatrixConstants::x_numAccumulators];
        uint8_t m_total[LogicMatrixConstants::x_numAccumulators];
        float m_pitch;
    };

    MatrixEvalResult EvalMatrix(InputVector inputVector);

    struct InputVectorIterator
    {
        uint8_t m_ordinal = 0;
        InputVector m_coMuteVector;
        size_t m_coMuteSize = 0;
        InputVector m_defaultVector;
        size_t m_forwardingIndices[LogicMatrixConstants::x_numInputs];

        InputVectorIterator(InputVector coMuteVector, InputVector defaultVector);

        InputVector Get();
        void Next();
        bool Done();
    };
    
    struct CoMuteSwitch
    {        
        void Init(rack::engine::Param* swtch)
        {
            m_switch = swtch;
        }
        
        bool IsCoMuted()
        {
            return m_switch->getValue() < 0.5;
        }
        
        rack::engine::Param* m_switch = nullptr;
    };

    struct CoMuteState
    {
        InputVector GetCoMuteVector()
        {
            using namespace LogicMatrixConstants;
            
            InputVector result;
            for (size_t i = 0; i < x_numInputs; ++i)
            {
                result.Set(i, m_switches[i].IsCoMuted());
            }

            return result;
        }

        void Init(rack::engine::Param* percentileKnob)
        {
            m_percentileKnob = percentileKnob;
        }
        
        CoMuteSwitch m_switches[LogicMatrixConstants::x_numInputs];
        rack::engine::Param* m_percentileKnob = nullptr;
    };

    struct Output
    {
        rack::engine::Output* m_mainOut = nullptr;
        rack::engine::Output* m_triggerOut = nullptr;
        rack::engine::Light* m_triggerLight = nullptr;
        rack::dsp::PulseGenerator m_pulseGen;
        float m_pitch = 0.0;
        CoMuteState m_coMuteState;

        MatrixEvalResult ComputePitch(LogicMatrix* matrix, InputVector defaultVector);
       
        void SetPitch(float pitch, float dt)
        {
            bool changedThisFrame = (pitch != m_pitch);
            m_pitch = pitch;
            m_mainOut->setVoltage(pitch);

            if (changedThisFrame)
            {
                m_pulseGen.trigger(0.01);
            }

            bool trig = m_pulseGen.process(dt);
            m_triggerOut->setVoltage(trig ? 5.f : 0.f);
            m_triggerLight->setBrightness(trig ? 1.f : 0.f);
        }

        InputVectorIterator GetInputVectorIterator(InputVector defaultVector)
        {
            return InputVectorIterator(m_coMuteState.GetCoMuteVector(), defaultVector);
        }
        
        void Init(
            rack::engine::Output* mainOut,
            rack::engine::Output* triggerOut,
            rack::engine::Light* triggerLight,
            rack::engine::Param* percentileKnob)
        {
            m_mainOut = mainOut;
            m_triggerOut = triggerOut;
            m_triggerLight = triggerLight;
            m_coMuteState.Init(percentileKnob);
        }
    };

    InputVector ProcessInputs();
    void ProcessOperations(InputVector defaultVector);
    void ProcessCVOuts(InputVector defaultVector);
    void ProcessOutputs(InputVector defaultVector, float dt);
    
	LogicMatrix();

    ~LogicMatrix()
    {
    }

    void process(const ProcessArgs& args) override;

    Input m_inputs[LogicMatrixConstants::x_numInputs];
    LogicOperation m_operations[LogicMatrixConstants::x_numOperations];
    Accumulator m_accumulators[LogicMatrixConstants::x_numAccumulators];
    Output m_outputs[LogicMatrixConstants::x_numAccumulators];
};
