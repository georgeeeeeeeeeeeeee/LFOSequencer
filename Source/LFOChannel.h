#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <cmath>
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

// Raw waveform value at phase p (0..1) -> output (-1..1)
inline float computeWaveValue(WaveShape shape, double p,
                              float lastRandomTarget, float nextRandomTarget, float sAndHValue)
{
    switch (shape)
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
            // Smooth interpolation between random targets
            return lastRandomTarget + (nextRandomTarget - lastRandomTarget) * static_cast<float>(p);

        case WaveShape::SampleAndHold:
            return sAndHValue;

        default:
            return 0.0f;
    }
}

inline double wrapPhase(double p) { return p - std::floor(p); }

struct LFOChannel
{
    // Parameters — written by the UI thread and state restore, read by the audio thread
    std::atomic<float>     frequencyHz   { 0.1f };   // default 0.1 Hz (10 second cycle)
    std::atomic<WaveShape> waveShape     { WaveShape::Sine };
    std::atomic<int>       midiNote      { 60 };     // C4
    std::atomic<int>       midiChannel   { 1 };      // 1-16
    std::atomic<int>       ccNumber      { 1 };      // mod wheel default
    std::atomic<float>     peakThreshold { 0.95f };  // how close to peak to trigger note
    std::atomic<int>       velocityMin   { 64 };
    std::atomic<int>       velocityMax   { 127 };
    std::atomic<bool>      noteEnabled   { true };
    std::atomic<bool>      ccEnabled     { true };
    std::atomic<bool>      active        { true };
    std::atomic<int>       noteLengthMs  { 100 };    // how long the note stays on
    std::atomic<float>     phaseOffset   { 0.0f };   // 0.0 to 1.0 (0° to 360°) — staggers timing between channels

    // Display state — published by the audio thread once per block, read by the UI
    std::atomic<double> displayPhase      { 0.0 };
    std::atomic<float>  displayValue      { 0.0f };
    std::atomic<float>  displayLastTarget { 0.0f };
    std::atomic<float>  displayNextTarget { 0.0f };
    std::atomic<float>  displaySAndH      { 0.0f };

    // Audio-thread state
    double phase            = 0.0;    // offset already applied, 0..1
    float  appliedOffset    = 0.0f;   // the phaseOffset value that phase currently includes
    float  currentValue     = 0.0f;   // -1 to 1 normalised
    float  prevValue        = 0.0f;
    bool   noteIsOn         = false;
    int    soundingNote     = 60;     // note and channel of the note that is on, so note-off
    int    soundingChannel  = 1;      // matches even if the parameters change meanwhile
    int    noteOffCountdown = 0;      // samples remaining until note off
    float  sAndHValue       = 0.0f;
    float  lastRandomTarget = 0.0f;
    float  nextRandomTarget = 0.0f;
    int    lastCCValue      = -1;     // -1 forces first emission; gates CC to value-changes only

    std::mt19937 rng{ std::random_device{}() };

    // Restart the cycle. A note still sounding is released on the next processed sample.
    void reset()
    {
        appliedOffset    = phaseOffset.load(std::memory_order_relaxed);
        phase            = wrapPhase(static_cast<double>(appliedOffset));
        currentValue     = 0.0f;
        prevValue        = 0.0f;
        sAndHValue       = 0.0f;
        lastRandomTarget = 0.0f;
        nextRandomTarget = randomFloat();
        lastCCValue      = -1;
        if (noteIsOn)
            noteOffCountdown = 0;
        publishDisplay();
    }

    // Restart the cycle at the channel's offset, keeping random state
    void sync(juce::MidiBuffer& midiOut, int sampleIndex)
    {
        releaseNote(midiOut, sampleIndex);
        appliedOffset = phaseOffset.load(std::memory_order_relaxed);
        phase         = wrapPhase(static_cast<double>(appliedOffset));
        prevValue     = 0.0f;
        currentValue  = valueAt(phase);
        publishDisplay();
    }

    void releaseNote(juce::MidiBuffer& midiOut, int sampleIndex)
    {
        if (!noteIsOn) return;
        midiOut.addEvent(juce::MidiMessage::noteOff(soundingChannel, soundingNote), sampleIndex);
        noteIsOn = false;
        noteOffCountdown = 0;
    }

    float randomFloat()
    {
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        return dist(rng);
    }

    // Audio thread
    float valueAt(double p) const
    {
        return computeWaveValue(waveShape.load(std::memory_order_relaxed), p,
                                lastRandomTarget, nextRandomTarget, sAndHValue);
    }

    // UI thread
    float displayValueAt(double p) const
    {
        return computeWaveValue(waveShape.load(), p, displayLastTarget.load(),
                                displayNextTarget.load(), displaySAndH.load());
    }

    void publishDisplay()
    {
        displayPhase.store(phase, std::memory_order_relaxed);
        displayValue.store(currentValue, std::memory_order_relaxed);
        displayLastTarget.store(lastRandomTarget, std::memory_order_relaxed);
        displayNextTarget.store(nextRandomTarget, std::memory_order_relaxed);
        displaySAndH.store(sAndHValue, std::memory_order_relaxed);
    }

    // Process one sample, optionally adding MIDI events to the buffer
    void processSample(double sampleRate, juce::MidiBuffer& midiOut, int sampleIndex)
    {
        if (!active.load(std::memory_order_relaxed))
        {
            releaseNote(midiOut, sampleIndex);
            return;
        }

        const auto shape = waveShape.load(std::memory_order_relaxed);

        // An offset change shifts the running phase; it isn't a cycle wrap, so it triggers nothing
        const float offset = phaseOffset.load(std::memory_order_relaxed);
        if (offset != appliedOffset)
        {
            phase = wrapPhase(phase + static_cast<double>(offset - appliedOffset));
            appliedOffset = offset;
        }

        prevValue = currentValue;

        // Advance phase
        const double phaseInc = frequencyHz.load(std::memory_order_relaxed) / sampleRate;
        phase += phaseInc;

        // Detect phase wrap (completed one cycle)
        bool cycleWrapped = false;
        if (phase >= 1.0)
        {
            phase = wrapPhase(phase);
            cycleWrapped = true;

            // For random wave: pick new targets
            if (shape == WaveShape::Random)
            {
                lastRandomTarget = nextRandomTarget;
                nextRandomTarget = randomFloat();
            }
            // For S&H: latch new value
            if (shape == WaveShape::SampleAndHold)
            {
                sAndHValue = randomFloat();
            }
        }

        currentValue = valueAt(phase);

        const int velMin = velocityMin.load(std::memory_order_relaxed);
        const int velMax = velocityMax.load(std::memory_order_relaxed);

        // --- MIDI CC output (continuous, shape-dependent, range VMin–VMax) ---
        if (ccEnabled.load(std::memory_order_relaxed))
        {
            float norm  = (currentValue + 1.0f) * 0.5f; // 0..1
            int ccVal   = juce::jlimit(0, 127,
                velMin + static_cast<int>(std::round(norm * (float)(velMax - velMin))));
            if (ccVal != lastCCValue)
            {
                auto msg = juce::MidiMessage::controllerEvent(midiChannel.load(std::memory_order_relaxed),
                                                              ccNumber.load(std::memory_order_relaxed), ccVal);
                midiOut.addEvent(msg, sampleIndex);
                lastCCValue = ccVal;
            }
        }

        // --- MIDI note trigger at peak ---
        if (noteEnabled.load(std::memory_order_relaxed))
        {
            bool atPeak = false;

            switch (shape)
            {
                case WaveShape::Sine:
                case WaveShape::Triangle:
                case WaveShape::Random:
                {
                    // Samples rarely land exactly on the peak, so a threshold at the top would
                    // almost never fire: cap it below 1.0 by more than one sample's movement
                    // near the peak (4 × phase step covers the triangle's slope).
                    const float threshold = juce::jmin(peakThreshold.load(std::memory_order_relaxed),
                                                       1.0f - (1.0e-3f + 4.0f * static_cast<float>(phaseInc)));
                    // Rising-edge detection: fires exactly once per cycle
                    atPeak = (prevValue < threshold && currentValue >= threshold);
                    break;
                }

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
                int velocity = velMin + static_cast<int>(velNorm * (velMax - velMin));
                velocity = juce::jlimit(1, 127, velocity);

                soundingNote    = midiNote.load(std::memory_order_relaxed);
                soundingChannel = midiChannel.load(std::memory_order_relaxed);
                midiOut.addEvent(juce::MidiMessage::noteOn(soundingChannel, soundingNote, (juce::uint8)velocity),
                                 sampleIndex);
                noteIsOn = true;
                noteOffCountdown = static_cast<int>((noteLengthMs.load(std::memory_order_relaxed) / 1000.0) * sampleRate);
            }
        }

        // Note-off countdown runs even if note output was switched off mid-note
        if (noteIsOn && --noteOffCountdown <= 0)
            releaseNote(midiOut, sampleIndex);
    }
};
