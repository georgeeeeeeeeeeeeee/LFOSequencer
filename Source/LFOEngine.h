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

    // Rate spread: set the active channels' rates as ratios of the first active
    // channel's rate, so one choice gives a polyrhythm. Follows with a Sync so the
    // cycles start together.
    enum class RateSpread
    {
        Multiples,      // x1, x2, x3, x4 ...
        Divisions,      // /1, /2, /3, /4 ...
        StepsHalf,      // x1, x1.5, x2, x2.5 ...
        StepsThird,     // x1, x1.33, x1.67, x2 ...
        StepsQuarter,   // x1, x1.25, x1.5, x1.75 ...
        StepsEighth,    // x1, x1.125, x1.25 ...
        Primes,         // x1, x1.3, x1.7, x2.3 ... (patterns that rarely line up)
        Deviate         // each within about 10% of the first, at random
    };

    static float rateSpreadRatio(RateSpread mode, int n, juce::Random& rng)
    {
        static constexpr float primes[NUM_CHANNELS] = { 1.0f, 1.3f, 1.7f, 2.3f, 2.9f, 3.1f, 3.7f, 4.1f };
        const float k = static_cast<float>(n);

        switch (mode)
        {
            case RateSpread::Multiples:    return 1.0f + k;
            case RateSpread::Divisions:    return 1.0f / (1.0f + k);
            case RateSpread::StepsHalf:    return 1.0f + k * 0.5f;
            case RateSpread::StepsThird:   return 1.0f + k / 3.0f;
            case RateSpread::StepsQuarter: return 1.0f + k * 0.25f;
            case RateSpread::StepsEighth:  return 1.0f + k * 0.125f;
            case RateSpread::Primes:       return primes[juce::jlimit(0, NUM_CHANNELS - 1, n)];
            case RateSpread::Deviate:      return n == 0 ? 1.0f : 0.9f + 0.2f * rng.nextFloat();
        }
        return 1.0f;
    }

    void spreadRates(RateSpread mode)
    {
        juce::Random rng;
        float base = 0.0f;
        int idx = 0;

        for (auto& ch : channels)
        {
            if (!ch.active) continue;

            if (idx == 0)
                base = ch.frequencyHz.load();
            else
                ch.frequencyHz = juce::jlimit(LFOChannel::kMinHz, LFOChannel::kMaxHz,
                                              base * rateSpreadRatio(mode, idx, rng));
            idx++;
        }

        if (idx > 1)
            requestSync();
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
