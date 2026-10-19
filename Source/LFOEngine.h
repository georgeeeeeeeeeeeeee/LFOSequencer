#pragma once

#include <atomic>
#include "LFOChannel.h"

class LFOEngine
{
public:
    static constexpr int NUM_CHANNELS = 8;

    LFOEngine()
    {
        for (int i = 0; i < NUM_CHANNELS; ++i)
        {
            channels[i].midiNote    = 60 + i;
            channels[i].frequencyHz = 0.1f * (i + 1);
            channels[i].ccNumber    = 20 + i;
            channels[i].midiChannel = 1;
            channels[i].active      = (i == 0);
            channels[i].phaseOffset = 0.0f;
        }
    }

    void prepareToPlay(double newSampleRate)
    {
        sampleRate = newSampleRate;
        for (auto& ch : channels)
            ch.reset();
    }

    void processBlock(juce::MidiBuffer& midiOut, int numSamples,
                      const juce::MidiBuffer* midiIn = nullptr)
    {
        // ── MIDI Learn: scan incoming MIDI for CC to auto-assign ──
        if (midiIn != nullptr)
        {
            for (const auto metadata : *midiIn)
            {
                auto msg = metadata.getMessage();
                if (msg.isController())
                {
                    for (auto& ch : channels)
                    {
                        if (ch.midiLearnActive)
                        {
                            ch.ccNumber = msg.getControllerNumber();
                            ch.midiChannel = msg.getChannel();
                            ch.midiLearnActive = false;
                            midiLearnCompleted = true;
                            midiLearnCompletedChannel = static_cast<int>(&ch - channels);
                            break; // first learning channel wins
                        }
                    }
                }
            }
        }

        // ── LFO processing ──
        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (auto& ch : channels)
                ch.processSample(sampleRate, midiOut, sample);
        }
    }

    // Sync: reset all phases to zero (offsets preserved)
    void syncAll()
    {
        for (auto& ch : channels)
        {
            ch.phase = 0.0;
            ch.prevValue = 0.0f;
            ch.currentValue = ch.computeWaveValue(static_cast<double>(ch.phaseOffset));
            ch.noteIsOn = false;
            ch.noteOffCountdown = 0;
        }
    }

    void resetAll()
    {
        for (auto& ch : channels)
            ch.reset();
    }

    // Auto-spread: evenly distribute phase offsets across active channels
    // 4 active → 0°, 90°, 180°, 270°
    void spreadPhases()
    {
        int activeCount = 0;
        for (const auto& ch : channels)
            if (ch.active) activeCount++;
        if (activeCount == 0) return;

        int idx = 0;
        for (auto& ch : channels)
        {
            if (ch.active)
            {
                ch.phaseOffset = static_cast<float>(idx) / static_cast<float>(activeCount);
                idx++;
            }
        }
    }

    LFOChannel& getChannel(int index)
    {
        jassert(index >= 0 && index < NUM_CHANNELS);
        return channels[index];
    }

    const LFOChannel& getChannel(int index) const
    {
        jassert(index >= 0 && index < NUM_CHANNELS);
        return channels[index];
    }

    LFOChannel channels[NUM_CHANNELS];

    // MIDI learn state (polled by UI — atomic for audio/UI thread safety)
    std::atomic<bool> midiLearnCompleted { false };
    std::atomic<int>  midiLearnCompletedChannel { -1 };

private:
    double sampleRate = 44100.0;
};
