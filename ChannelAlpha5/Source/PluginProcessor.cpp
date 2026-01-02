// ============================================================================
// Source/PluginProcessor.cpp
// ============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BusShared.h" // Include bus shared memory

ChannelAlpha2Processor::ChannelAlpha2Processor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
#endif
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize with stereo channel layout
    muteGain.setCurrentAndTargetValue(1.0f);
    faderGain.setCurrentAndTargetValue(1.0f);
    panValue.setCurrentAndTargetValue(0.0f);
    // Initialize DDX3216 parameter smoothers
    emuAmountSmoother.setCurrentAndTargetValue(1.0f);
    preEmphasisSmoother.setCurrentAndTargetValue(0.0f);
    harmonicsSmoother.setCurrentAndTargetValue(0.0f);
    intersampleSmoother.setCurrentAndTargetValue(0.0f);
    saturationSmoother.setCurrentAndTargetValue(0.25f);
    noiseFloorSmoother.setCurrentAndTargetValue(-96.0f);
    inputGainSmoother.setCurrentAndTargetValue(0.0f);
    // Track initial channel ID
    currentChannelID = getChannelID();
    // Register with bus shared memory
    BusShared::getInstance().registerWriter(currentChannelID);
    // Add parameter listener for channel ID changes
    apvts.addParameterListener(PARAM_CHANNEL_ID, this);
}

ChannelAlpha2Processor::~ChannelAlpha2Processor() {
    // Unregister from bus shared memory
    BusShared::getInstance().unregisterWriter(currentChannelID);
    apvts.removeParameterListener(PARAM_CHANNEL_ID, this);
    oversampler.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ChannelAlpha2Processor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // We support ONLY stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    // Input and output must match
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

juce::AudioProcessorValueTreeState::ParameterLayout ChannelAlpha2Processor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ PARAM_CHANNEL_ID, 1 },
        "Channel ID",
        1, 32, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ PARAM_FADER, 1 },
        "Fader",
        juce::NormalisableRange<float>(-60.0f, 10.0f, 0.1f, 2.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ PARAM_PAN, 1 },
        "Pan",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
        0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ PARAM_EMU_AMOUNT, 1 },
        "Emu Amount",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f, 1.0f),
        1.0f,
        juce::AudioParameterFloatAttributes().withLabel("x")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ PARAM_PRE_EMPHASIS, 1 },
        "Pre Emphasis",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ PARAM_HARMONICS, 1 },
        "Harmonics",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ PARAM_INTERSAMPLE, 1 },
        "Intersample Mod",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ PARAM_NOISE_FLOOR, 1 },
        "Noise Floor",
        juce::NormalisableRange<float>(-120.0f, -60.0f, 0.1f),
        -96.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ PARAM_INPUT_GAIN, 1 },
        "Input Gain",
        juce::NormalisableRange<float>(-20.0f, 60.0f, 0.1f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    // Button parameters
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ PARAM_SELECT, 1 },
        "Select",
        false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ PARAM_AUTO_REC, 1 },
        "Auto Record",
        false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ PARAM_SOLO, 1 },
        "Solo",
        false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ PARAM_MUTE, 1 },
        "Mute",
        false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ PARAM_DDX_EMULATION, 1 },
        "DDX3216 Emulation",
        false));
    // Bus send enable parameter
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ PARAM_BUS_SEND_ENABLED, 1 },
        "Bus Send",
        true));
    return { params.begin(), params.end() };
}

void ChannelAlpha2Processor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Smooth parameter changes
    muteGain.reset(sampleRate, 0.05);
    faderGain.reset(sampleRate, 0.05);
    panValue.reset(sampleRate, 0.05);
    // Smooth DDX3216 parameters
    emuAmountSmoother.reset(sampleRate, 0.03);
    preEmphasisSmoother.reset(sampleRate, 0.03);
    harmonicsSmoother.reset(sampleRate, 0.03);
    intersampleSmoother.reset(sampleRate, 0.03);
    saturationSmoother.reset(sampleRate, 0.03);
    noiseFloorSmoother.reset(sampleRate, 0.03);
    inputGainSmoother.reset(sampleRate, 0.03);
    muteGain.setCurrentAndTargetValue(muted ? 0.0f : 1.0f);
    // Prepare EQ chains for exactly 2 channels (stereo)
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2; // ALWAYS 2 channels for stereo processing
    preEQChain.prepare(spec);
    postEQChain.prepare(spec);
    // Initialize filters
    *preEQChain.get<0>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        static_cast<float>(sampleRate), 22.0f, 0.707f);
    // Warmth shelf (subtle low boost)
    *preEQChain.get<1>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        static_cast<float>(sampleRate), 180.0f, 0.65f, juce::Decibels::decibelsToGain(1.2f));
    // Air shelf (subtle high boost)
    *preEQChain.get<2>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        static_cast<float>(sampleRate), 9500.0f, 0.75f, juce::Decibels::decibelsToGain(0.6f));
    // Pre-emphasis will be set dynamically
    *preEQChain.get<3>().coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        static_cast<float>(sampleRate), 6000.0f, 1.0f, 1.0f);
    // Post-EQ gentle tilt
    *postEQChain.get<0>().coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        static_cast<float>(sampleRate), 12000.0f, 0.9f, juce::Decibels::decibelsToGain(-0.4f));
    oversamplerPrepared = false;
    // Initialize exactly 2 random generators for noise
    channelSeeds.clear();
    noiseHPFStates.clear();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(1, 0x7FFFFFFF);
    for (int i = 0; i < 2; ++i) {
        uint32_t seed = dist(gen);
        channelSeeds.push_back(seed);
        noiseHPFStates.push_back(0.0f);
    }
    // High-pass filter for noise (300Hz cutoff)
    float cutoff = 300.0f;
    float rc = 1.0f / (2.0f * 3.1415926535f * cutoff);
    float dt = 1.0f / static_cast<float>(sampleRate);
    noiseHPFCoeff = dt / (rc + dt);
}

void ChannelAlpha2Processor::releaseResources() {
    preEQChain.reset();
    postEQChain.reset();
    if (oversampler && oversamplerPrepared) {
        oversampler->reset();
    }
    std::fill(noiseHPFStates.begin(), noiseHPFStates.end(), 0.0f);
}

void ChannelAlpha2Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels beyond 2 (stereo)
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    if (buffer.getNumSamples() == 0) return;

    // Get RAW parameter values for enable checks
    float rawFaderDB = apvts.getRawParameterValue(PARAM_FADER)->load();
    float rawPan = apvts.getRawParameterValue(PARAM_PAN)->load();
    float rawEmuAmount = apvts.getRawParameterValue(PARAM_EMU_AMOUNT)->load();

    // Update state from APVTS parameters
    muted = apvts.getRawParameterValue(PARAM_MUTE)->load() > 0.5f;
    soloed = apvts.getRawParameterValue(PARAM_SOLO)->load() > 0.5f;
    selected = apvts.getRawParameterValue(PARAM_SELECT)->load() > 0.5f;
    autoRec = apvts.getRawParameterValue(PARAM_AUTO_REC)->load() > 0.5f;
    ddxEmulation = apvts.getRawParameterValue(PARAM_DDX_EMULATION)->load() > 0.5f;
    busSendEnabled = apvts.getRawParameterValue(PARAM_BUS_SEND_ENABLED)->load() > 0.5f;

    // Update smoothers (for internal DSP use)
    faderGain.setTargetValue(juce::Decibels::decibelsToGain(rawFaderDB));
    panValue.setTargetValue(rawPan);
    muteGain.setTargetValue(muted ? 0.0f : 1.0f);
    emuAmountSmoother.setTargetValue(rawEmuAmount);
    preEmphasisSmoother.setTargetValue(apvts.getRawParameterValue(PARAM_PRE_EMPHASIS)->load());
    harmonicsSmoother.setTargetValue(apvts.getRawParameterValue(PARAM_HARMONICS)->load());
    intersampleSmoother.setTargetValue(apvts.getRawParameterValue(PARAM_INTERSAMPLE)->load());
    noiseFloorSmoother.setTargetValue(apvts.getRawParameterValue(PARAM_NOISE_FLOOR)->load());
    inputGainSmoother.setTargetValue(apvts.getRawParameterValue(PARAM_INPUT_GAIN)->load());

    // Process panning and level
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float currentFader = faderGain.getNextValue(); // Advance smoother
        float currentPan = panValue.getNextValue(); // Advance smoother
        float currentMute = muteGain.getNextValue(); // Advance smoother
        float totalGain = currentFader * currentMute;

        // Stereo processing with constant power panning
        if (buffer.getNumChannels() >= 2) {
            float panAngle = (currentPan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
            float leftPanGain = std::cos(panAngle);
            float rightPanGain = std::sin(panAngle);
            buffer.setSample(0, sample, buffer.getSample(0, sample) * totalGain * leftPanGain);
            buffer.setSample(1, sample, buffer.getSample(1, sample) * totalGain * rightPanGain);
        }
    }

    // Apply DDX3216 emulation if enabled (check RAW value)
    if (ddxEmulation && rawEmuAmount > 0.001f) {
        processDDX3216(buffer);
    }

    // Add this section to your processBlock, right before the bus send:
    // Write processed stereo audio to the bus for this channel if enabled
    if (busSendEnabled) {
        int channelID = getChannelID();
        // DEBUG: Log every 1000 blocks
        static int debugCounter = 0;
        if (++debugCounter >= 1000) {
            debugCounter = 0;
            DBG("Channel Alpha: Writing to channel " << channelID
                << " | Samples: " << buffer.getNumSamples()
                << " | L[0]: " << buffer.getSample(0, 0)
                << " | R[0]: " << buffer.getSample(1, 0)
                << " | Shared Mem OK: " << (BusShared::getInstance().isInitialized() ? "YES" : "NO"));
        }
        BusShared::getInstance().writeToChannel(
            channelID,
            buffer.getReadPointer(0),               // Left channel
            buffer.getNumChannels() >= 2 ? buffer.getReadPointer(1) : nullptr, // Right channel
            buffer.getNumSamples()
        );
    }
}

// Handle parameter changes for channel ID switching
void ChannelAlpha2Processor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == PARAM_CHANNEL_ID)
    {
        int newChannelID = static_cast<int>(newValue);
        // Unregister from old channel and register to new channel
        if (newChannelID != currentChannelID)
        {
            BusShared::getInstance().unregisterWriter(currentChannelID);
            BusShared::getInstance().registerWriter(newChannelID);
            currentChannelID = newChannelID;
        }
    }
}

void ChannelAlpha2Processor::processDDX3216(juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return;
    juce::dsp::AudioBlock<float> block(buffer);
    // Ensure we only process 2 channels
    if (block.getNumChannels() > 2) {
        block = block.getSubsetChannelBlock(0, 2);
    }
    // Advance smoothers ONCE per block
    float currentEmuAmount = emuAmountSmoother.getNextValue();
    float currentPreEmphasis = preEmphasisSmoother.getNextValue();
    float currentHarmonics = harmonicsSmoother.getNextValue();
    float currentIntersample = intersampleSmoother.getNextValue();
    float currentNoiseFloor = noiseFloorSmoother.getNextValue();
    float currentInputGain = inputGainSmoother.getNextValue();
    float currentSaturation = saturationSmoother.getNextValue();

    // Apply pre-emphasis filter (based on knob)
    float preEmphasisGainDB = currentPreEmphasis * 8.0f; // 0-8dB boost
    *preEQChain.get<3>().coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        spec.sampleRate, 6000.0f, 1.0f, juce::Decibels::decibelsToGain(preEmphasisGainDB));

    // Apply pre-EQ chain (HPF, warmth, air, pre-emphasis)
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    preEQChain.process(ctx);

    // Apply saturation (scaled by emulation amount)
    applySaturation(buffer, currentSaturation * currentEmuAmount);

    // Apply harmonics (if knob is turned up)
    if (currentHarmonics > 0.001f) {
        applyHarmonics(buffer, currentHarmonics);
    }

    // Apply noise (affected by noise floor and input gain knobs)
    applyNoise(buffer, currentNoiseFloor, currentInputGain, currentEmuAmount);

    // Apply post-EQ (gentle tilt)
    postEQChain.process(ctx);

    // Apply intersample modulation (if knob is turned up)
    if (currentIntersample > 0.001f) {
        applyIntersampleModulation(buffer, currentIntersample);
    }
}

inline float ChannelAlpha2Processor::fastTanh(float x) const {
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float ChannelAlpha2Processor::softClip(float x) const {
    if (x > 1.0f) return 1.0f - (1.0f / (3.0f * x * x));
    if (x < -1.0f) return -1.0f + (1.0f / (3.0f * x * x));
    return x - (x * x * x) / 3.0f;
}

inline float ChannelAlpha2Processor::hardClip(float x, float threshold) const {
    return juce::jlimit(-threshold, threshold, x);
}

inline float ChannelAlpha2Processor::generateWhiteNoise(uint32_t& seed) const {
    seed = seed * 1664525u + 1013904223u;
    int32_t signedSeed = static_cast<int32_t>(seed);
    return static_cast<float>(signedSeed) / 2147483648.0f;
}

void ChannelAlpha2Processor::applySaturation(juce::AudioBuffer<float>& buffer, float amount) {
    if (amount <= 0.001f) return;
    float drive = 1.10f + amount * 0.15f;
    // Process exactly 2 channels (we only support stereo)
    for (int ch = 0; ch < 2; ++ch) {
        auto* data = buffer.getWritePointer(ch);
        int numSamples = buffer.getNumSamples();
        for (int i = 0; i < numSamples; ++i) {
            float x = data[i] * drive;
            float tanhSat = fastTanh(x);
            float softSat = softClip(x);
            float hardSat = hardClip(x, 0.9f + amount * 0.1f);
            float mixed = 0.5f * tanhSat + 0.3f * softSat + 0.2f * hardSat;
            float wetMix = juce::jlimit(0.0f, 1.0f, amount);
            data[i] = data[i] * (1.0f - wetMix) + mixed * wetMix;
        }
    }
}

void ChannelAlpha2Processor::applyHarmonics(juce::AudioBuffer<float>& buffer, float amount) {
    float harmonicsGain = amount * 0.5f;
    // Process exactly 2 channels (we only support stereo)
    for (int ch = 0; ch < 2; ++ch) {
        auto* data = buffer.getWritePointer(ch);
        int numSamples = buffer.getNumSamples();
        for (int i = 0; i < numSamples; ++i) {
            float x = data[i];
            // Add harmonic distortion (even and odd harmonics)
            float x2 = x * x * harmonicsGain * 0.3f;
            float x3 = x * x * x * harmonicsGain * 0.2f;
            float x5 = x * x * x * x * x * harmonicsGain * 0.1f;
            data[i] += x2 + x3 + x5;
        }
    }
}

void ChannelAlpha2Processor::applyNoise(juce::AudioBuffer<float>& buffer, float noiseFloorDB, float inputGainDB, float emuAmount) {
    if (noiseFloorDB <= -119.0f && inputGainDB <= -10.0f) return;
    float baseNoiseGain = juce::Decibels::decibelsToGain(noiseFloorDB);
    float gainNoise = juce::Decibels::decibelsToGain(inputGainDB * 0.1f);
    float totalNoiseGain = baseNoiseGain * gainNoise * emuAmount * 0.01f;
    if (totalNoiseGain < 0.000001f) return;

    // Process exactly 2 channels (we only support stereo)
    for (int ch = 0; ch < 2 && ch < (int)channelSeeds.size() && ch < (int)noiseHPFStates.size(); ++ch) {
        auto* data = buffer.getWritePointer(ch);
        int numSamples = buffer.getNumSamples();
        uint32_t& seed = channelSeeds[ch];
        float& hpfState = noiseHPFStates[ch];
        for (int i = 0; i < numSamples; ++i) {
            float whiteNoise = generateWhiteNoise(seed);
            hpfState = noiseHPFCoeff * hpfState + (1.0f - noiseHPFCoeff) * whiteNoise;
            float filteredNoise = whiteNoise - hpfState;
            data[i] += filteredNoise * totalNoiseGain;
        }
    }
}

void ChannelAlpha2Processor::applyIntersampleModulation(juce::AudioBuffer<float>& buffer, float amount) {
    if (!oversamplerPrepared) {
        oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
            2, 2, // 2x oversampling
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
            false);
        oversampler->initProcessing(spec.maximumBlockSize);
        oversamplerPrepared = true;
    }

    float threshold = 1.0f - (amount * 0.3f);
    float hardness = 0.5f + amount * 0.5f;

    juce::dsp::AudioBlock<float> block(buffer);
    if (block.getNumChannels() > 2) {
        block = block.getSubsetChannelBlock(0, 2);
    }

    // Upsample
    juce::dsp::AudioBlock<float> oversampledBlock = oversampler->processSamplesUp(block);

    // Process at oversampled rate (only 2 channels)
    for (int ch = 0; ch < (int)oversampledBlock.getNumChannels(); ++ch) {
        auto* data = oversampledBlock.getChannelPointer(ch);
        int numSamples = (int)oversampledBlock.getNumSamples();
        for (int i = 0; i < numSamples; ++i) {
            float sample = data[i];
            // Apply soft clipping with threshold
            if (std::abs(sample) > threshold) {
                float over = std::abs(sample) - threshold;
                float sign = sample > 0 ? 1.0f : -1.0f;
                if (hardness > 0.8f) {
                    data[i] = sign * threshold;
                }
                else {
                    float knee = over * (1.0f - hardness);
                    data[i] = sign * (threshold + knee);
                }
            }
            // Add subtle cubic distortion
            data[i] += data[i] * data[i] * data[i] * 0.02f * amount;
        }
    }

    // Downsample
    oversampler->processSamplesDown(block);
}

void ChannelAlpha2Processor::setMuted(bool shouldMute) { muted = shouldMute; }
void ChannelAlpha2Processor::setSoloed(bool shouldSolo) { soloed = shouldSolo; }
void ChannelAlpha2Processor::setSelected(bool shouldSelect) { selected = shouldSelect; }
void ChannelAlpha2Processor::setAutoRec(bool shouldAutoRec) { autoRec = shouldAutoRec; }
void ChannelAlpha2Processor::setDDXEmulation(bool shouldEnable) {
    ddxEmulation = shouldEnable;
    if (!shouldEnable) {
        emuAmountSmoother.setTargetValue(0.0f);
    }
}
void ChannelAlpha2Processor::setBusSendEnabled(bool shouldEnable) { busSendEnabled = shouldEnable; }

int ChannelAlpha2Processor::getChannelID() const {
    return static_cast<int>(apvts.getRawParameterValue(PARAM_CHANNEL_ID)->load());
}

juce::AudioProcessorEditor* ChannelAlpha2Processor::createEditor() {
    return new ChannelAlpha2Editor(*this);
}

void ChannelAlpha2Processor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    state.setProperty("muted", muted.load(), nullptr);
    state.setProperty("soloed", soloed.load(), nullptr);
    state.setProperty("selected", selected.load(), nullptr);
    state.setProperty("autoRec", autoRec.load(), nullptr);
    state.setProperty("ddxEmulation", ddxEmulation.load(), nullptr);
    state.setProperty("busSendEnabled", busSendEnabled.load(), nullptr);
    state.setProperty("pluginWidth", pluginWidth, nullptr);
    state.setProperty("pluginHeight", pluginHeight, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ChannelAlpha2Processor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr) {
        if (xmlState->hasTagName(apvts.state.getType())) {
            auto state = juce::ValueTree::fromXml(*xmlState);
            apvts.replaceState(state);
            muted = state.getProperty("muted", false);
            soloed = state.getProperty("soloed", false);
            selected = state.getProperty("selected", false);
            autoRec = state.getProperty("autoRec", false);
            busSendEnabled = state.getProperty("busSendEnabled", true);
            pluginWidth = state.getProperty("pluginWidth", 160);
            pluginHeight = state.getProperty("pluginHeight", 530);
            bool ddxState = state.getProperty("ddxEmulation", false);
            setDDXEmulation(ddxState);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new ChannelAlpha2Processor();
}