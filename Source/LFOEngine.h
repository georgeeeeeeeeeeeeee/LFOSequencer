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

    void processBlock(juce::MidiBuffer& midiOut, int numSamples)
    {
        // ── Sync / Reset requested by the UI ──
        const bool resetNow = resetRequested.exchange(false);
        const bool syncNow  = syncRequested.exchange(false);
        for (auto& ch : channels)
        {
            if (resetNow)
            {
                ch.releaseNote(midiOut, 0);
                ch.reset();
            }
            else if (syncNow)
            {
                ch.sync(midiOut, 0);
            }
        }

        // ── LFO processing ──
        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (auto& ch : channels)
                ch.processSample(sampleRate, midiOut, sample);
        }

        for (auto& ch : channels)
            ch.publishDisplay();
    }

    void releaseAllNotes(juce::MidiBuffer& midiOut)
    {
        for (auto& ch : channels)
            ch.releaseNote(midiOut, 0);
    }

    // UI thread: carried out on the audio thread at the start of the next block

    // Sync: reset all phases to zero (offsets preserved)
    void requestSync()  { syncRequested.store(true); }

    void requestReset() { resetRequested.store(true); }

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

private:
    double sampleRate = 44100.0;
    std::atomic<bool> syncRequested  { false };
    std::atomic<bool> resetRequested { false };
};
