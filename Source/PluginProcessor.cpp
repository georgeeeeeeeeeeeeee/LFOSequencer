#include "PluginProcessor.h"
#include "PluginEditor.h"

LFOSequencerProcessor::LFOSequencerProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput ("Input",  juce::AudioChannelSet::stereo(), false)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), false))
{
}

LFOSequencerProcessor::~LFOSequencerProcessor() {}

void LFOSequencerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    lastBlockSize = samplesPerBlock;
    engine.prepareToPlay(sampleRate);
}

void LFOSequencerProcessor::releaseResources() {}

void LFOSequencerProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Capture incoming MIDI for MIDI learn before clearing
    juce::MidiBuffer incomingMidi(midiMessages);

    // Clear incoming MIDI (we generate, not pass through)
    midiMessages.clear();

    int numSamples = buffer.getNumSamples();
    if (numSamples <= 0) numSamples = lastBlockSize;
    if (numSamples <= 0) return;
    engine.processBlock(midiMessages, numSamples, &incomingMidi);
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
        chXml->setAttribute("active",        ch.active);
        chXml->setAttribute("frequencyHz",   ch.frequencyHz);
        chXml->setAttribute("waveShape",     static_cast<int>(ch.waveShape));
        chXml->setAttribute("midiNote",      ch.midiNote);
        chXml->setAttribute("midiChannel",   ch.midiChannel);
        chXml->setAttribute("ccNumber",      ch.ccNumber);
        chXml->setAttribute("peakThreshold", ch.peakThreshold);
        chXml->setAttribute("velocityMin",   ch.velocityMin);
        chXml->setAttribute("velocityMax",   ch.velocityMax);
        chXml->setAttribute("noteEnabled",   ch.noteEnabled);
        chXml->setAttribute("ccEnabled",     ch.ccEnabled);
        chXml->setAttribute("noteLengthMs",  ch.noteLengthMs);
        chXml->setAttribute("phaseOffset",   ch.phaseOffset);
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

        auto& ch = engine.getChannel(idx);
        ch.active        = chXml->getBoolAttribute("active", true);
        ch.frequencyHz   = static_cast<float>(chXml->getDoubleAttribute("frequencyHz", 0.1));
        ch.waveShape     = static_cast<WaveShape>(chXml->getIntAttribute("waveShape", 0));
        ch.midiNote      = chXml->getIntAttribute("midiNote", 60);
        ch.midiChannel   = chXml->getIntAttribute("midiChannel", 1);
        ch.ccNumber      = chXml->getIntAttribute("ccNumber", 1);
        ch.peakThreshold = static_cast<float>(chXml->getDoubleAttribute("peakThreshold", 0.95));
        ch.velocityMin   = chXml->getIntAttribute("velocityMin", 64);
        ch.velocityMax   = chXml->getIntAttribute("velocityMax", 127);
        ch.noteEnabled   = chXml->getBoolAttribute("noteEnabled", true);
        ch.ccEnabled     = chXml->getBoolAttribute("ccEnabled", true);
        ch.noteLengthMs  = chXml->getIntAttribute("noteLengthMs", 100);
        ch.phaseOffset   = static_cast<float>(chXml->getDoubleAttribute("phaseOffset", 0.0));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LFOSequencerProcessor();
}
