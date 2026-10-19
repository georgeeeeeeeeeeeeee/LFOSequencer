#pragma once

#include <JuceHeader.h>
#include <random>

enum class WaveShape
{
    Sine = 0,
    Square,
    Triangle,
    Saw,
    Random,
    SampleAndHold,
    NumShapes
};

inline juce::String waveShapeName(WaveShape s)
{
    switch (s)
    {
        case WaveShape::Sine:          return "Sine";
        case WaveShape::Square:        return "Square";
        case WaveShape::Triangle:      return "Triangle";
        case WaveShape::Saw:           return "Saw";
        case WaveShape::Random:        return "Random";
        case WaveShape::SampleAndHold: return "S&H";
        default:                       return "?";
    }
}

struct LFOChannel
{
    // Parameters
    float frequencyHz   = 0.1f;   // default 0.1 Hz (10 second cycle)
    WaveShape waveShape = WaveShape::Sine;
    int   midiNote      = 60;     // C4
    int   midiChannel   = 1;      // 1-16
    int   ccNumber      = 1;      // mod wheel default
    float peakThreshold = 0.95f;  // how close to peak to trigger note
    int   velocityMin   = 64;
    int   velocityMax   = 127;
    bool  noteEnabled   = true;
    bool  ccEnabled     = true;
    bool  active        = true;
    int   noteLengthMs  = 100;    // how long the note stays on
    float phaseOffset   = 0.0f;   // 0.0 to 1.0 (0° to 360°) — staggers timing between channels
    bool  midiLearnActive = false; // when true, next incoming CC sets this channel's ccNumber

    // State
    double phase        = 0.0;
    float  currentValue = 0.0f;   // -1 to 1 normalised
    float  prevValue    = 0.0f;
    bool   noteIsOn     = false;
    int    noteOffCountdown = 0;  // samples remaining until note off
    float  sAndHValue   = 0.0f;
    float  lastRandomTarget = 0.0f;
    float  nextRandomTarget = 0.0f;
    int    lastCCValue  = -1;     // -1 forces first emission; gates CC to value-changes only

    std::mt19937 rng{ std::random_device{}() };

    void reset()
    {
        phase = 0.0;
        currentValue = 0.0f;
        prevValue = 0.0f;
        noteIsOn = false;
        noteOffCountdown = 0;
        sAndHValue = 0.0f;
        lastRandomTarget = 0.0f;
        nextRandomTarget = randomFloat();
        lastCCValue = -1;
    }

    float randomFloat()
    {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        return dist(rng);
    }

    // Compute raw waveform value at current phase (0..1) -> output (-1..1)
    float computeWaveValue(double p) const
    {
        switch (waveShape)
        {
            case WaveShape::Sine:
                return static_cast<float>(std::sin(p * juce::MathConstants<double>::twoPi));

            case WaveShape::Square:
                return p < 0.5 ? 1.0f : -1.0f;

            case WaveShape::Triangle:
                if (p < 0.25)       return static_cast<float>(p * 4.0);
                else if (p < 0.75)  return static_cast<float>(2.0 - p * 4.0);
                else                return static_cast<float>(p * 4.0 - 4.0);

            case WaveShape::Saw:
                return static_cast<float>(2.0 * p - 1.0);

            case WaveShape::Random:
            {
                // Smooth interpolation between random targets
                float t = static_cast<float>(p);
                return lastRandomTarget + (nextRandomTarget - lastRandomTarget) * t;
            }

            case WaveShape::SampleAndHold:
                return sAndHValue;

            default:
                return 0.0f;
        }
    }

    // Process one sample, optionally adding MIDI events to the buffer
    void processSample(double sampleRate, juce::MidiBuffer& midiOut, int sampleIndex)
    {
        if (!active) return;

        prevValue = currentValue;
        double prevPhase = phase;

        // Advance phase
        double phaseInc = frequencyHz / sampleRate;
        phase += phaseInc;

        // Detect phase wrap (completed one cycle)
        bool cycleWrapped = false;
        if (phase >= 1.0)
        {
            phase -= 1.0;
            cycleWrapped = true;

            // For random wave: pick new targets
            if (waveShape == WaveShape::Random)
            {
                lastRandomTarget = nextRandomTarget;
                nextRandomTarget = randomFloat();
            }
            // For S&H: latch new value
            if (waveShape == WaveShape::SampleAndHold)
            {
                sAndHValue = randomFloat();
            }
        }

        currentValue = computeWaveValue(std::fmod(phase + static_cast<double>(phaseOffset), 1.0));

        // --- MIDI CC output (continuous, shape-dependent, range VMin–VMax) ---
        if (ccEnabled)
        {
            float norm  = (currentValue + 1.0f) * 0.5f; // 0..1
            int ccVal   = juce::jlimit(0, 127,
                velocityMin + static_cast<int>(std::round(norm * (float)(velocityMax - velocityMin))));
            if (ccVal != lastCCValue)
            {
                auto msg = juce::MidiMessage::controllerEvent(midiChannel, ccNumber, ccVal);
                midiOut.addEvent(msg, sampleIndex);
                lastCCValue = ccVal;
            }
        }

        // --- MIDI note trigger at peak ---
        if (noteEnabled)
        {
            // Detect peak: previous value was rising, current is falling (or threshold crossed)
            bool atPeak = false;

            switch (waveShape)
            {
                case WaveShape::Sine:
                case WaveShape::Triangle:
                case WaveShape::Random:
                    // Rising-edge detection: fires exactly once per cycle
                    atPeak = (prevValue < peakThreshold && currentValue >= peakThreshold);
                    break;

                case WaveShape::Square:
                    // Trigger on rising edge
                    atPeak = (prevValue < 0.0f && currentValue >= 0.5f);
                    break;

                case WaveShape::Saw:
                    // Trigger at wrap
                    atPeak = cycleWrapped;
                    break;

                case WaveShape::SampleAndHold:
                    // Trigger on cycle wrap (new value latched)
                    atPeak = cycleWrapped;
                    break;

                default:
                    break;
            }

            if (atPeak && !noteIsOn)
            {
                // Calculate velocity from current wave amplitude
                float velNorm = (currentValue + 1.0f) * 0.5f; // 0..1
                int velocity = velocityMin + static_cast<int>(velNorm * (velocityMax - velocityMin));
                velocity = juce::jlimit(1, 127, velocity);

                auto noteOn = juce::MidiMessage::noteOn(midiChannel, midiNote, (juce::uint8)velocity);
                midiOut.addEvent(noteOn, sampleIndex);
                noteIsOn = true;
                noteOffCountdown = static_cast<int>((noteLengthMs / 1000.0) * sampleRate);
            }

            // Handle note off countdown
            if (noteIsOn)
            {
                noteOffCountdown--;
                if (noteOffCountdown <= 0)
                {
                    auto noteOff = juce::MidiMessage::noteOff(midiChannel, midiNote);
                    midiOut.addEvent(noteOff, sampleIndex);
                    noteIsOn = false;
                }
            }
        }
    }
};
