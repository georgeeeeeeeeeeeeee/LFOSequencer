#include "PluginProcessor.h"
#include "PluginEditor.h"

LFOSequencerProcessor::LFOSequencerProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput ("Input",  juce::AudioChannelSet::stereo(), false)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), false))
{
}

LFOSequencerProcessor::~LFOSequencerProcessor() {}

void LFOSequencerProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    engine.prepareToPlay(sampleRate);
}

void LFOSequencerProcessor::releaseResources() {}

void LFOSequencerProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear incoming MIDI (we generate, not pass through)
    midiMessages.clear();

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0) return;
    engine.processBlock(midiMessages, numSamples);
}

void LFOSequencerProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer& midiMessages)
{
    AudioProcessor::processBlockBypassed(buffer, midiMessages);

    // Don't leave notes hanging while bypassed
    if (buffer.getNumSamples() > 0)
        engine.releaseAllNotes(midiMessages);
}

juce::AudioProcessorEditor* LFOSequencerProcessor::createEditor()
{
    return new LFOSequencerEditor(*this);
}

void LFOSequencerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = std::make_unique<juce::XmlElement>("LFOSequencerState");
    xml->setAttribute("version", 2);

    for (int i = 0; i < LFOEngine::NUM_CHANNELS; ++i)
    {
        auto& ch = engine.getChannel(i);
        auto* chXml = xml->createNewChildElement("Channel");
        chXml->setAttribute("index",         i);
        chXml->setAttribute("active",        ch.active.load());
        chXml->setAttribute("frequencyHz",   ch.frequencyHz.load());
        chXml->setAttribute("waveShape",     static_cast<int>(ch.waveShape.load()));
        chXml->setAttribute("midiNote",      ch.midiNote.load());
        chXml->setAttribute("midiChannel",   ch.midiChannel.load());
        chXml->setAttribute("ccNumber",      ch.ccNumber.load());
        chXml->setAttribute("peakThreshold", ch.peakThreshold.load());
        chXml->setAttribute("velocityMin",   ch.velocityMin.load());
        chXml->setAttribute("velocityMax",   ch.velocityMax.load());
        chXml->setAttribute("noteEnabled",   ch.noteEnabled.load());
        chXml->setAttribute("ccEnabled",     ch.ccEnabled.load());
        chXml->setAttribute("noteLengthMs",  ch.noteLengthMs.load());
        chXml->setAttribute("phaseOffset",   ch.phaseOffset.load());
    }

    copyXmlToBinary(*xml, destData);
}

void LFOSequencerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr || !xml->hasTagName("LFOSequencerState"))
        return;

    // Only version 2 exists so far; newer files are read the same way, unknown attributes ignored

    std::array<const juce::XmlElement*, LFOEngine::NUM_CHANNELS> channelXml {};
    for (auto* chXml : xml->getChildWithTagNameIterator("Channel"))
    {
        int idx = chXml->getIntAttribute("index", -1);
        if (idx >= 0 && idx < LFOEngine::NUM_CHANNELS)
            channelXml[(size_t) idx] = chXml;
    }

    // A missing channel or attribute falls back to the engine's own default for that channel.
    // Values are clamped to the ranges the UI allows, and non-finite ones replaced, so a damaged
    // or hand-edited session can't produce invalid MIDI.
    static const LFOEngine defaults;

    for (int i = 0; i < LFOEngine::NUM_CHANNELS; ++i)
    {
        const auto* chXml = channelXml[(size_t) i];
        const auto& d  = defaults.getChannel(i);
        auto& ch       = engine.getChannel(i);

        auto getInt = [chXml](const char* name, int def)
        {
            return chXml != nullptr ? chXml->getIntAttribute(name, def) : def;
        };
        auto getBool = [chXml](const char* name, bool def)
        {
            return chXml != nullptr ? chXml->getBoolAttribute(name, def) : def;
        };
        auto getFloat = [chXml](const char* name, float def)
        {
            const double v = chXml != nullptr ? chXml->getDoubleAttribute(name, def) : def;
            return std::isfinite(v) ? static_cast<float>(v) : def;
        };

        ch.active        = getBool("active", d.active.load());
        ch.frequencyHz   = juce::jlimit(0.01f, 100.0f, getFloat("frequencyHz", d.frequencyHz.load()));
        ch.waveShape     = static_cast<WaveShape>(juce::jlimit(0, static_cast<int>(WaveShape::NumShapes) - 1,
                                                               getInt("waveShape", static_cast<int>(d.waveShape.load()))));
        ch.midiNote      = juce::jlimit(0, 127, getInt("midiNote",    d.midiNote.load()));
        ch.midiChannel   = juce::jlimit(1, 16,  getInt("midiChannel", d.midiChannel.load()));
        ch.ccNumber      = juce::jlimit(0, 127, getInt("ccNumber",    d.ccNumber.load()));
        ch.peakThreshold = juce::jlimit(0.0f, 1.0f, getFloat("peakThreshold", d.peakThreshold.load()));
        ch.velocityMin   = juce::jlimit(0, 127, getInt("velocityMin", d.velocityMin.load()));
        ch.velocityMax   = juce::jlimit(0, 127, getInt("velocityMax", d.velocityMax.load()));
        ch.noteEnabled   = getBool("noteEnabled", d.noteEnabled.load());
        ch.ccEnabled     = getBool("ccEnabled",   d.ccEnabled.load());
        ch.noteLengthMs  = juce::jlimit(1, 5000, getInt("noteLengthMs", d.noteLengthMs.load()));
        ch.phaseOffset   = static_cast<float>(wrapPhase(getFloat("phaseOffset", d.phaseOffset.load())));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LFOSequencerProcessor();
}
