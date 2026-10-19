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

    for (auto* chXml : xml->getChildWithTagNameIterator("Channel"))
    {
        int idx = chXml->getIntAttribute("index", -1);
        if (idx < 0 || idx >= LFOEngine::NUM_CHANNELS)
            continue;

        // Clamp to the ranges the UI allows, so a damaged or hand-edited session can't produce invalid MIDI
        auto& ch = engine.getChannel(idx);
        ch.active        = chXml->getBoolAttribute("active", true);
        ch.frequencyHz   = juce::jlimit(0.01f, 100.0f, static_cast<float>(chXml->getDoubleAttribute("frequencyHz", 0.1)));
        ch.waveShape     = static_cast<WaveShape>(juce::jlimit(0, static_cast<int>(WaveShape::NumShapes) - 1,
                                                               chXml->getIntAttribute("waveShape", 0)));
        ch.midiNote      = juce::jlimit(0, 127, chXml->getIntAttribute("midiNote", 60));
        ch.midiChannel   = juce::jlimit(1, 16,  chXml->getIntAttribute("midiChannel", 1));
        ch.ccNumber      = juce::jlimit(0, 127, chXml->getIntAttribute("ccNumber", 1));
        ch.peakThreshold = juce::jlimit(0.0f, 1.0f, static_cast<float>(chXml->getDoubleAttribute("peakThreshold", 0.95)));
        ch.velocityMin   = juce::jlimit(0, 127, chXml->getIntAttribute("velocityMin", 64));
        ch.velocityMax   = juce::jlimit(0, 127, chXml->getIntAttribute("velocityMax", 127));
        ch.noteEnabled   = chXml->getBoolAttribute("noteEnabled", true);
        ch.ccEnabled     = chXml->getBoolAttribute("ccEnabled", true);
        ch.noteLengthMs  = juce::jlimit(1, 5000, chXml->getIntAttribute("noteLengthMs", 100));
        ch.phaseOffset   = static_cast<float>(wrapPhase(chXml->getDoubleAttribute("phaseOffset", 0.0)));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LFOSequencerProcessor();
}
