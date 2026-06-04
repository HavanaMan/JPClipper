#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr int currentStateVersion = 10;
    constexpr float ispAlertThresholdDb = 0.0f;

    const juce::Identifier stateVersionProperty{ "stateVersion" };
    const juce::Identifier currentPresetNameProperty{ "currentPresetName" };
    const juce::Identifier editorWidthProperty{ "editorWidth" };
    const juce::Identifier editorHeightProperty{ "editorHeight" };
    const juce::Identifier abCompareTreeName{ "AB_COMPARE" };
    const juce::Identifier activeSlotProperty{ "activeSlot" };

    constexpr int defaultEditorWidth = 1020;
    constexpr int defaultEditorHeight = 600;
    constexpr int minEditorWidth = 765;
    constexpr int minEditorHeight = 450;
    constexpr int maxEditorWidth = 1530;
    constexpr int maxEditorHeight = 900;
    constexpr const char* userPresetExtension = ".jpclipperpreset";

    struct FactoryPreset
    {
        const char* name;
        float inputGainDb;
        float thresholdDb;
        float outputGainDb;
        float ceilingDb;
        bool autoGain;
        float mixPercent;
        bool dcBlocker;
        bool deltaListen;
        bool bypass;
        int oversampling;
        int phaseMode;
        int mode;
        int processMode;
        float softKneePercent;
        int liftMode;
        float colorAmountPercent;
        float stereoLinkPercent;
        bool match;
        int scopeMode;
        float scopeZoom;
    };

    const std::array<FactoryPreset, 6> factoryPresets{ {
        { "DEFAULT",        0.0f, -6.0f,  0.0f, -0.1f, false,100.0f, false,false, false, 0, 0, 1, 0, 50.0f, 0,  0.0f, 100.0f, false, 1, 3.0f },
        { "SOFT GLUE",      4.0f, -8.0f, -0.5f, -0.3f, true, 100.0f, true, false, false, 2, 0, 1, 0, 70.0f, 1, 18.0f, 100.0f, false, 0, 3.0f },
        { "DRUM CLIP",      8.0f, -6.0f, -1.0f, -0.2f, true,  78.0f, true, false, false, 3, 0, 0, 0, 20.0f, 2, 28.0f,  75.0f, false, 1, 3.0f },
        { "LOUD HARD",     10.0f, -4.5f, -1.2f, -0.1f, true, 100.0f, true, false, false, 3, 0, 0, 0,  0.0f, 3, 20.0f, 100.0f, false, 0, 3.0f },
        { "PARALLEL COLOR", 6.0f, -7.5f,  0.0f, -0.2f, true,  52.0f, true, false, false, 2, 1, 1, 0, 80.0f, 1, 35.0f,  85.0f, false, 1, 3.0f },
        { "CLEAN CEILING",  0.0f, -1.5f,  0.0f, -0.1f, false,100.0f, true, false, false, 4, 0, 0, 0, 10.0f, 0,  0.0f, 100.0f, false, 0, 3.0f }
    } };

    float getLiftAmountForMode(int mode)
    {
        switch (juce::jlimit(0, 3, mode))
        {
        case 1:  return 0.16f; // BODY
        case 2:  return 0.30f; // DENS
        case 3:  return 0.46f; // PUSH
        default: return 0.0f;
        }
    }

    float catmullRom(float p0, float p1, float p2, float p3, float t)
    {
        const float t2 = t * t;
        const float t3 = t2 * t;

        return 0.5f * ((2.0f * p1)
            + (-p0 + p2) * t
            + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
            + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
    }

    enum ProcessMode
    {
        processStereo = 0,
        processMid = 1,
        processSide = 2,
        processMidSide = 3
    };

    bool shouldProcessDomainChannel(int channel, int numChannels, int processMode)
    {
        if (numChannels < 2)
            return true;

        switch (juce::jlimit(0, 3, processMode))
        {
        case processMid:  return channel == 0;
        case processSide: return channel == 1;
        default:          return true;
        }
    }

    void encodeStereoToMidSide(juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumChannels() < 2)
            return;

        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float l = left[i];
            const float r = right[i];

            left[i] = 0.5f * (l + r);
            right[i] = 0.5f * (l - r);
        }
    }

    void decodeMidSideToStereo(juce::AudioBuffer<float>& buffer)
    {
        if (buffer.getNumChannels() < 2)
            return;

        auto* mid = buffer.getWritePointer(0);
        auto* side = buffer.getWritePointer(1);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float m = mid[i];
            const float s = side[i];

            mid[i] = m + s;
            side[i] = m - s;
        }
    }

    float meanSquareToApproxShortTermLufs(double meanSquare)
    {
        if (meanSquare <= 1.0e-12)
            return -60.0f;

        // BS.1770 uses K-weighting. This lightweight meter intentionally uses
        // the standard LUFS offset on the post-processing mean square so it is
        // stable and realtime-safe without adding another filter stage.
        return juce::jlimit(-60.0f, 12.0f,
            -0.691f + 10.0f * std::log10((float)meanSquare));
    }
}

JPClipperAudioProcessor::JPClipperAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, &undoManager, "PARAMETERS", createParameters())
{
    resetWaveform();
    resetDcBlocker();
    refreshUserPresets();
    initialiseABSlots();
}

juce::AudioProcessorValueTreeState::ParameterLayout JPClipperAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "inputGain", "Input Trim",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "threshold", "Threshold",
        juce::NormalisableRange<float>(-24.0f, 0.0f, 0.01f),
        -6.0f));


    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "outputGain", "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.01f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ceiling", "Ceiling",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.01f),
        -0.1f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "autoGain", "Auto Gain",
        false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "match", "Match",
        false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        100.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "dcBlocker", "DC Blocker",
        false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "deltaListen", "Delta",
        false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "bypass", "Bypass",
        false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "oversampling", "Oversampling",
        juce::StringArray{ "Off", "2x", "4x", "8x", "16x", "32x" },
        0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "phaseMode", "Phase Mode",
        juce::StringArray{ "Linear Phase", "Min Phase" },
        0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "mode", "Mode",
        juce::StringArray{ "Hard", "Soft" },
        1));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "processMode", "Process Mode",
        juce::StringArray{ "Stereo", "Mid", "Side", "M+S" },
        0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "softKnee", "Soft Knee",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        50.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "liftMode", "Lift",
        juce::StringArray{ "Off", "Body", "Dense", "Push" },
        0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "color", "Color",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "stereoLink", "Stereo Link",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        100.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "scopeMode", "Scope",
        juce::StringArray{ "OUT", "IN/OUT", "DELTA", "PEAKS" },
        1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "scopeZoom", "Scope Time",
        juce::NormalisableRange<float>(1.0f, 8.0f, 1.0f),
        3.0f));

    return { params.begin(), params.end() };
}

void JPClipperAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentMaxBlockSize = samplesPerBlock;
    currentNumChannels = juce::jmax(1, getTotalNumOutputChannels());

    inputLevel.store(0.0f);
    outputLevel.store(0.0f);
    clipAmount.store(0.0f);
    clipReductionDb.store(0.0f);
    inputLeftLevel.store(0.0f);
    inputRightLevel.store(0.0f);
    outputLeftLevel.store(0.0f);
    outputRightLevel.store(0.0f);
    inputRmsDb.store(-60.0f);
    outputRmsDb.store(-60.0f);
    outputTruePeakDb.store(-60.0f);
    outputShortTermLufs.store(-60.0f);
    shortTermLufsMeanSquare = 0.0;
    matchGainDb.store(0.0f);
    outputIspActive.store(false);
    waveformWriteIndex.store(0);
    truePeakPreviousOutput.fill(0.0f);

    resetWaveform();
    resetDcBlocker();
    resetParameterSmoothers();
    resizeParameterSmoothingBuffers(samplesPerBlock);
    rebuildOversamplingEngines();
    currentOversamplingLatencySamples = 0;
    resizeDryLatencyBuffer();

    const int oversamplingChoice = (int)apvts.getRawParameterValue("oversampling")->load();
    const int phaseModeChoice = (int)apvts.getRawParameterValue("phaseMode")->load();
    currentOversamplingChoice = -1;
    currentPhaseModeChoice = -1;
    updateOversamplingIfNeeded(oversamplingChoice, phaseModeChoice);
}

void JPClipperAudioProcessor::releaseResources()
{
    oversampling = nullptr;

    for (auto& engine : oversamplingEngines)
        engine.reset();

    currentOversamplingLatencySamples = 0;
    maxOversamplingLatencySamples = 0;
    resetDryLatencyBuffer();
    setLatencySamples(0);
}

void JPClipperAudioProcessor::resetParameterSmoothers()
{
    const double sampleRate = juce::jmax(1.0, currentSampleRate);

    inputGainSmoothed.reset(sampleRate, parameterSmoothingTimeSeconds);
    thresholdSmoothed.reset(sampleRate, parameterSmoothingTimeSeconds);
    autoGainSmoothed.reset(sampleRate, parameterSmoothingTimeSeconds);
    outputGainSmoothed.reset(sampleRate, parameterSmoothingTimeSeconds);
    ceilingGainSmoothed.reset(sampleRate, parameterSmoothingTimeSeconds);
    mixSmoothed.reset(sampleRate, parameterSmoothingTimeSeconds);
    softKneeSmoothed.reset(sampleRate, parameterSmoothingTimeSeconds);
    liftAmountSmoothed.reset(sampleRate, parameterSmoothingTimeSeconds);
    colorAmountSmoothed.reset(sampleRate, parameterSmoothingTimeSeconds);
    stereoLinkSmoothed.reset(sampleRate, parameterSmoothingTimeSeconds);
    matchGainSmoothed.reset(sampleRate, 1.250);
    bypassSmoothed.reset(sampleRate, 0.020);

    const float inputGainDb = apvts.getRawParameterValue("inputGain")->load();
    const float thresholdDb = apvts.getRawParameterValue("threshold")->load();
    const float outputGainDb = apvts.getRawParameterValue("outputGain")->load();
    const float ceilingDb = apvts.getRawParameterValue("ceiling")->load();
    const float mix = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("mix")->load() / 100.0f);
    const float softKnee = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("softKnee")->load() / 100.0f);
    const float liftAmount = getLiftAmountForMode((int)apvts.getRawParameterValue("liftMode")->load());
    const float colorAmount = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("color")->load() / 100.0f);
    const float stereoLink = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("stereoLink")->load() / 100.0f);
    const bool autoGainEnabled = apvts.getRawParameterValue("autoGain")->load() > 0.5f;
    const float bypass = apvts.getRawParameterValue("bypass")->load() > 0.5f ? 1.0f : 0.0f;

    const float inputGain = juce::Decibels::decibelsToGain(inputGainDb);
    const float threshold = juce::jmax(0.00001f, juce::Decibels::decibelsToGain(thresholdDb));
    const float outputGain = juce::Decibels::decibelsToGain(outputGainDb);
    const float ceilingGain = juce::jmax(0.00001f, juce::Decibels::decibelsToGain(ceilingDb));
    const float autoGain = autoGainEnabled
        ? juce::Decibels::decibelsToGain(-inputGainDb)
        : 1.0f;

    inputGainSmoothed.setCurrentAndTargetValue(inputGain);
    thresholdSmoothed.setCurrentAndTargetValue(threshold);
    autoGainSmoothed.setCurrentAndTargetValue(autoGain);
    outputGainSmoothed.setCurrentAndTargetValue(outputGain);
    ceilingGainSmoothed.setCurrentAndTargetValue(ceilingGain);
    mixSmoothed.setCurrentAndTargetValue(mix);
    softKneeSmoothed.setCurrentAndTargetValue(softKnee);
    liftAmountSmoothed.setCurrentAndTargetValue(liftAmount);
    colorAmountSmoothed.setCurrentAndTargetValue(colorAmount);
    stereoLinkSmoothed.setCurrentAndTargetValue(stereoLink);
    matchTargetGain = 1.0f;
    matchGainSmoothed.setCurrentAndTargetValue(1.0f);
    bypassSmoothed.setCurrentAndTargetValue(bypass);
}

void JPClipperAudioProcessor::resizeParameterSmoothingBuffers(int numSamples)
{
    const size_t bufferSize = (size_t)juce::jmax(1, numSamples);

    inputGainValues.resize(bufferSize);
    thresholdValues.resize(bufferSize);
    autoGainValues.resize(bufferSize);
    outputGainValues.resize(bufferSize);
    ceilingGainValues.resize(bufferSize);
    mixValues.resize(bufferSize);
    softKneeValues.resize(bufferSize);
    liftAmountValues.resize(bufferSize);
    colorAmountValues.resize(bufferSize);
    stereoLinkValues.resize(bufferSize);
    matchGainValues.resize(bufferSize);
    bypassValues.resize(bufferSize);
}

void JPClipperAudioProcessor::fillParameterSmoothingBuffers(int numSamples,
    float inputGainTarget,
    float thresholdTarget,
    float autoGainTarget,
    float outputGainTarget,
    float ceilingGainTarget,
    float mixTarget,
    float softKneeTarget,
    float liftAmountTarget,
    float colorAmountTarget,
    float stereoLinkTarget,
    float bypassTarget)
{
    resizeParameterSmoothingBuffers(numSamples);

    inputGainSmoothed.setTargetValue(inputGainTarget);
    thresholdSmoothed.setTargetValue(juce::jmax(0.00001f, thresholdTarget));
    autoGainSmoothed.setTargetValue(autoGainTarget);
    outputGainSmoothed.setTargetValue(outputGainTarget);
    ceilingGainSmoothed.setTargetValue(juce::jmax(0.00001f, ceilingGainTarget));
    mixSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, mixTarget));
    softKneeSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, softKneeTarget));
    liftAmountSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, liftAmountTarget));
    colorAmountSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, colorAmountTarget));
    stereoLinkSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, stereoLinkTarget));
    bypassSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, bypassTarget));

    for (int i = 0; i < numSamples; ++i)
    {
        inputGainValues[(size_t)i] = inputGainSmoothed.getNextValue();
        thresholdValues[(size_t)i] = thresholdSmoothed.getNextValue();
        autoGainValues[(size_t)i] = autoGainSmoothed.getNextValue();
        outputGainValues[(size_t)i] = outputGainSmoothed.getNextValue();
        ceilingGainValues[(size_t)i] = ceilingGainSmoothed.getNextValue();
        mixValues[(size_t)i] = juce::jlimit(0.0f, 1.0f, mixSmoothed.getNextValue());
        softKneeValues[(size_t)i] = juce::jlimit(0.0f, 1.0f, softKneeSmoothed.getNextValue());
        liftAmountValues[(size_t)i] = juce::jlimit(0.0f, 1.0f, liftAmountSmoothed.getNextValue());
        colorAmountValues[(size_t)i] = juce::jlimit(0.0f, 1.0f, colorAmountSmoothed.getNextValue());
        stereoLinkValues[(size_t)i] = juce::jlimit(0.0f, 1.0f, stereoLinkSmoothed.getNextValue());
        bypassValues[(size_t)i] = juce::jlimit(0.0f, 1.0f, bypassSmoothed.getNextValue());
    }
}

void JPClipperAudioProcessor::updateClipReductionMetrics(float inputSample, float clippedSample, float clipDelta)
{
    blockMaxClipping = juce::jmax(blockMaxClipping, clipDelta);

    const float absInput = std::abs(inputSample);
    const float absClipped = std::abs(clippedSample);

    if (absInput <= 1.0e-7f || absClipped >= absInput)
        return;

    const float reductionRatio = juce::jlimit(1.0e-7f, 1.0f, absClipped / absInput);
    const float reductionDb = -juce::Decibels::gainToDecibels(reductionRatio, -120.0f);

    blockMaxReductionDb = juce::jmax(blockMaxReductionDb, reductionDb);
}

bool JPClipperAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
        && !layouts.getMainInputChannelSet().isDisabled();
}

void JPClipperAudioProcessor::resetWaveform()
{
    waveformInputBlockMin = 0.0f;
    waveformInputBlockMax = 0.0f;
    waveformClippedBlockMin = 0.0f;
    waveformClippedBlockMax = 0.0f;
    waveformRemovedBlockMin = 0.0f;
    waveformRemovedBlockMax = 0.0f;
    waveformBlockCounter = 0;
    waveformWriteIndex.store(0, std::memory_order_release);

    for (auto& sample : waveformInputMin)
        sample.store(0.0f);

    for (auto& sample : waveformInputMax)
        sample.store(0.0f);

    for (auto& sample : waveformClippedMin)
        sample.store(0.0f);

    for (auto& sample : waveformClippedMax)
        sample.store(0.0f);

    for (auto& sample : waveformRemovedMin)
        sample.store(0.0f);

    for (auto& sample : waveformRemovedMax)
        sample.store(0.0f);
}

void JPClipperAudioProcessor::resetDcBlocker()
{
    dcX1.fill(0.0f);
    dcY1.fill(0.0f);

    const double sr = juce::jmax(1.0, currentSampleRate);
    const double cutoffHz = 20.0;
    dcR = (float)std::exp(-2.0 * juce::MathConstants<double>::pi * cutoffHz / sr);
}

float JPClipperAudioProcessor::processDcBlocker(float x, int channel)
{
    const int ch = juce::jlimit(0, maxDcChannels - 1, channel);

    const float y = x - dcX1[(size_t)ch] + dcR * dcY1[(size_t)ch];

    dcX1[(size_t)ch] = x;
    dcY1[(size_t)ch] = y;

    return y;
}

void JPClipperAudioProcessor::rebuildOversamplingEngines()
{
    oversampling = nullptr;
    maxOversamplingLatencySamples = 0;

    for (auto& engine : oversamplingEngines)
        engine.reset();

    const size_t numChannels = (size_t)juce::jmax(1, currentNumChannels);
    const size_t maxBlockSize = (size_t)juce::jmax(1, currentMaxBlockSize);

    for (int phase = 0; phase < phaseModeChoiceCount; ++phase)
    {
        const auto filterType = phase == 0
            ? juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple
            : juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR;

        for (int choice = 1; choice < oversamplingChoiceCount; ++choice)
        {
            const size_t factorExponent = (size_t)choice;
            auto engine = std::make_unique<juce::dsp::Oversampling<float>>(
                numChannels,
                factorExponent,
                filterType,
                true,
                true);

            engine->initProcessing(maxBlockSize);
            engine->reset();

            maxOversamplingLatencySamples = juce::jmax(
                maxOversamplingLatencySamples,
                (int)std::round(engine->getLatencyInSamples()));

            oversamplingEngines[(size_t)getOversamplingEngineIndex(choice, phase)] = std::move(engine);
        }
    }
}

int JPClipperAudioProcessor::getOversamplingEngineIndex(int oversamplingChoice, int phaseModeChoice) const
{
    const int choice = juce::jlimit(0, oversamplingChoiceCount - 1, oversamplingChoice);
    const int phase = juce::jlimit(0, phaseModeChoiceCount - 1, phaseModeChoice);

    return phase * oversamplingChoiceCount + choice;
}

void JPClipperAudioProcessor::updateOversamplingIfNeeded(int oversamplingChoice, int phaseModeChoice)
{
    oversamplingChoice = juce::jlimit(0, oversamplingChoiceCount - 1, oversamplingChoice);
    phaseModeChoice = juce::jlimit(0, phaseModeChoiceCount - 1, phaseModeChoice);

    if (oversamplingChoice == currentOversamplingChoice && phaseModeChoice == currentPhaseModeChoice)
        return;

    currentOversamplingChoice = oversamplingChoice;
    currentPhaseModeChoice = phaseModeChoice;

    if (oversamplingChoice == 0)
    {
        oversampling = nullptr;
        currentOversamplingLatencySamples = 0;
        resetDryLatencyBuffer();
        resetDcBlocker();
        setLatencySamples(0);
        return;
    }

    auto* selectedEngine = oversamplingEngines[(size_t)getOversamplingEngineIndex(oversamplingChoice, phaseModeChoice)].get();

    if (selectedEngine == nullptr)
    {
        oversampling = nullptr;
        currentOversamplingLatencySamples = 0;
        resetDryLatencyBuffer();
        resetDcBlocker();
        setLatencySamples(0);
        return;
    }

    oversampling = selectedEngine;
    oversampling->reset();

    currentOversamplingLatencySamples = (int)std::round(oversampling->getLatencyInSamples());
    resetDryLatencyBuffer();
    resetDcBlocker();
    setLatencySamples(currentOversamplingLatencySamples);
}

void JPClipperAudioProcessor::resizeDryLatencyBuffer()
{
    const int numChannels = juce::jmax(1, currentNumChannels);
    const int latencyCapacity = juce::jmax(currentOversamplingLatencySamples, maxOversamplingLatencySamples);
    const int bufferLength = juce::jmax(1, latencyCapacity + currentMaxBlockSize + 1);

    dryLatencyBuffer.resize((size_t)numChannels);
    dryLatencyWritePositions.resize((size_t)numChannels, 0);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        dryLatencyBuffer[(size_t)ch].assign((size_t)bufferLength, 0.0f);
        dryLatencyWritePositions[(size_t)ch] = 0;
    }
}

void JPClipperAudioProcessor::resetDryLatencyBuffer()
{
    for (auto& channelBuffer : dryLatencyBuffer)
        std::fill(channelBuffer.begin(), channelBuffer.end(), 0.0f);

    std::fill(dryLatencyWritePositions.begin(), dryLatencyWritePositions.end(), 0);
}

float JPClipperAudioProcessor::getLatencyCompensatedDrySample(int channel, float inputSample)
{
    if (currentOversamplingLatencySamples <= 0 || dryLatencyBuffer.empty())
        return inputSample;

    const int ch = juce::jlimit(0, (int)dryLatencyBuffer.size() - 1, channel);
    auto& delayBuffer = dryLatencyBuffer[(size_t)ch];

    if (delayBuffer.empty())
        return inputSample;

    auto& writePosition = dryLatencyWritePositions[(size_t)ch];
    const int bufferLength = (int)delayBuffer.size();

    if (bufferLength <= currentOversamplingLatencySamples)
        return inputSample;

    if (writePosition >= bufferLength)
        writePosition = 0;

    int readPosition = writePosition - currentOversamplingLatencySamples;
    if (readPosition < 0)
        readPosition += bufferLength;

    const float delayedSample = delayBuffer[(size_t)readPosition];

    delayBuffer[(size_t)writePosition] = inputSample;
    writePosition = (writePosition + 1) % bufferLength;

    return delayedSample;
}

float JPClipperAudioProcessor::processClipperSample(float inputSample,
    float threshold,
    float softKnee,
    int mode,
    float& clipDelta) const
{
    const float localThreshold = juce::jmax(0.00001f, threshold);
    const float absInput = std::abs(inputSample);
    const float sign = inputSample < 0.0f ? -1.0f : 1.0f;

    float clippedAbs = absInput;

    switch (mode)
    {
    case 0:
        clippedAbs = juce::jmin(absInput, localThreshold);
        break;

    case 1:
    default:
    {
        // Soft knee clipper.
        // 0% knee is hard clipping. Higher values begin the bend earlier and reach
        // the flat ceiling later, without ever boosting the signal inside the knee.
        const float kneeFraction = juce::jlimit(0.0f, 1.0f, softKnee);

        // Keep THRESHOLD = 0 dB as a true neutral pass-through point.
        // The previous linear knee width made KNEE = 100% start bending around
        // -20 dB even when the threshold was fully open, so IN 0 / OUT 0 did
        // not return the same level. Scale the knee down as threshold approaches
        // 0 dB: full knee at -12 dB and below, no knee at 0 dB.
        const float thresholdDb = juce::Decibels::gainToDecibels(localThreshold, -144.0f);
        const float thresholdDistanceFromZero = juce::jlimit(0.0f, 1.0f, -thresholdDb / 12.0f);
        const float effectiveKneeFraction = kneeFraction * thresholdDistanceFromZero;
        const float kneeWidth = localThreshold * 0.90f * effectiveKneeFraction;

        if (kneeWidth <= 1.0e-6f)
        {
            clippedAbs = juce::jmin(absInput, localThreshold);
        }
        else
        {
            const float kneeStart = juce::jmax(0.0f, localThreshold - kneeWidth);
            const float kneeEnd = localThreshold + kneeWidth;
            const float kneeRange = juce::jmax(1.0e-6f, kneeEnd - kneeStart);

            if (absInput <= kneeStart)
            {
                clippedAbs = absInput;
            }
            else if (absInput >= kneeEnd)
            {
                clippedAbs = localThreshold;
            }
            else
            {
                const float t = juce::jlimit(0.0f, 1.0f, (absInput - kneeStart) / kneeRange);
                const float t2 = t * t;
                const float t3 = t2 * t;

                const float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
                const float h10 = t3 - 2.0f * t2 + t;
                const float h01 = -2.0f * t3 + 3.0f * t2;

                clippedAbs = h00 * kneeStart + h10 * kneeRange + h01 * localThreshold;
                clippedAbs = juce::jmin(clippedAbs, absInput);
            }
        }

        break;
    }
    }

    const float clipped = sign * clippedAbs;
    clipDelta = std::abs(inputSample - clipped);

    return clipped;
}

float JPClipperAudioProcessor::processLiftSample(float inputSample,
    float threshold,
    float liftAmount) const
{
    if (liftAmount <= 0.0001f)
        return inputSample;

    const float localThreshold = juce::jmax(0.00001f, threshold);
    const float absInput = std::abs(inputSample);

    if (absInput <= 1.0e-8f)
        return inputSample;

    // Lift is a pre-clipper waveshaper: it lifts material below the clip
    // threshold, but peaks above threshold must still reach the clipper so
    // GR, delta listen and the red waveform crests remain truthful.
    if (absInput >= localThreshold)
        return inputSample;

    const float norm = absInput / localThreshold;
    const float room = 1.0f - norm;
    const float lift = liftAmount * room * room;
    const float shapedAbs = juce::jmin(localThreshold, absInput * (1.0f + lift));

    return inputSample < 0.0f ? -shapedAbs : shapedAbs;
}


float JPClipperAudioProcessor::processColorSample(float clippedSample,
    float threshold,
    float colorAmount) const
{
    const float amount = juce::jlimit(0.0f, 1.0f, colorAmount);

    if (amount <= 0.0001f)
        return clippedSample;

    const float localThreshold = juce::jmax(0.00001f, threshold);

    // Musical post-clipper colour:
    // use a curved control law, a lower maximum drive and a capped wet blend.
    // This keeps small COLOR values subtle and prevents 100% from becoming a
    // brutal second clipper after the main threshold stage.
    const float colour = amount * amount;
    const float normalised = clippedSample / localThreshold;
    const float drive = juce::jmap(colour, 0.0f, 1.0f, 1.0f, 2.05f);
    const float normaliser = std::tanh(drive);
    const float shaped = normaliser > 1.0e-6f
        ? std::tanh(normalised * drive) / normaliser
        : normalised;

    const float saturated = shaped * localThreshold;
    const float blend = juce::jlimit(0.0f, 0.78f, colour * 0.78f);

    return clippedSample + (saturated - clippedSample) * blend;
}

void JPClipperAudioProcessor::processWetBuffer(juce::AudioBuffer<float>& buffer,
    const std::vector<float>& inputGainBuffer,
    const std::vector<float>& thresholdBuffer,
    const std::vector<float>& liftAmountBuffer,
    const std::vector<float>& colorAmountBuffer,
    const std::vector<float>& autoGainBuffer,
    const std::vector<float>& softKneeBuffer,
    const std::vector<float>& stereoLinkBuffer,
    int mode,
    bool collectWaveform,
    bool deltaListen,
    int processMode)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        const size_t index = (size_t)i;
        const float inputGain = inputGainBuffer[index];
        const float threshold = thresholdBuffer[index];
        const float liftAmount = liftAmountBuffer[index];
        const float colorAmount = colorAmountBuffer[index];
        const float autoGain = autoGainBuffer[index];
        const float softKnee = softKneeBuffer[index];
        const float stereoLink = stereoLinkBuffer[index];

        float maxLiftedAbs = 0.0f;

        if (stereoLink > 0.0001f && numChannels > 1)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                if (!shouldProcessDomainChannel(ch, numChannels, processMode))
                    continue;

                auto* data = buffer.getWritePointer(ch);
                const float x = data[i] * inputGain;
                const float lifted = processLiftSample(x, threshold, liftAmount);
                maxLiftedAbs = juce::jmax(maxLiftedAbs, std::abs(lifted));
            }
        }

        float linkedGain = 1.0f;

        if (maxLiftedAbs > 1.0e-8f)
        {
            float linkedClipDelta = 0.0f;
            const float linkedClippedAbs = std::abs(processClipperSample(maxLiftedAbs, threshold, softKnee, mode, linkedClipDelta));
            linkedGain = juce::jlimit(0.0f, 1.0f, linkedClippedAbs / maxLiftedAbs);
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);

            if (!shouldProcessDomainChannel(ch, numChannels, processMode))
                continue;

            const float x = data[i] * inputGain;
            const float lifted = processLiftSample(x, threshold, liftAmount);

            blockMaxInput = juce::jmax(blockMaxInput, std::abs(x));

            float clipDelta = 0.0f;
            const float independentClipped = processClipperSample(lifted, threshold, softKnee, mode, clipDelta);
            const float linkedClipped = lifted * linkedGain;
            const float clipped = independentClipped + (linkedClipped - independentClipped) * stereoLink;
            clipDelta = std::abs(lifted - clipped);
            const float coloured = processColorSample(clipped, threshold, colorAmount);
            updateClipReductionMetrics(lifted, clipped, clipDelta);

            const float wetOutput = coloured * autoGain;
            data[i] = deltaListen ? (lifted - wetOutput) : wetOutput;

            const int waveformChannel = (numChannels > 1 && juce::jlimit(0, 3, processMode) == processSide) ? 1 : 0;

            if (collectWaveform && ch == waveformChannel)
                pushWaveformSample(lifted, clipped, coloured);
        }
    }
}

void JPClipperAudioProcessor::pushWaveformSample(float inputSample, float clippedSample, float outputSample)
{
    const float removedSample = inputSample - clippedSample;

    waveformInputBlockMin = juce::jmin(waveformInputBlockMin, inputSample);
    waveformInputBlockMax = juce::jmax(waveformInputBlockMax, inputSample);

    waveformClippedBlockMin = juce::jmin(waveformClippedBlockMin, outputSample);
    waveformClippedBlockMax = juce::jmax(waveformClippedBlockMax, outputSample);

    waveformRemovedBlockMin = juce::jmin(waveformRemovedBlockMin, removedSample);
    waveformRemovedBlockMax = juce::jmax(waveformRemovedBlockMax, removedSample);

    ++waveformBlockCounter;

    if (waveformBlockCounter < waveformSamplesPerPointCurrent)
        return;

    const int writeIndex = waveformWriteIndex.load(std::memory_order_relaxed);
    const int index = writeIndex % waveformSize;

    waveformInputMin[(size_t)index].store(waveformInputBlockMin, std::memory_order_relaxed);
    waveformInputMax[(size_t)index].store(waveformInputBlockMax, std::memory_order_relaxed);
    waveformClippedMin[(size_t)index].store(waveformClippedBlockMin, std::memory_order_relaxed);
    waveformClippedMax[(size_t)index].store(waveformClippedBlockMax, std::memory_order_relaxed);
    waveformRemovedMin[(size_t)index].store(waveformRemovedBlockMin, std::memory_order_relaxed);
    waveformRemovedMax[(size_t)index].store(waveformRemovedBlockMax, std::memory_order_relaxed);

    waveformWriteIndex.store((writeIndex + 1) % waveformSize, std::memory_order_release);

    waveformInputBlockMin = 0.0f;
    waveformInputBlockMax = 0.0f;
    waveformClippedBlockMin = 0.0f;
    waveformClippedBlockMax = 0.0f;
    waveformRemovedBlockMin = 0.0f;
    waveformRemovedBlockMax = 0.0f;
    waveformBlockCounter = 0;
}

void JPClipperAudioProcessor::copyWaveformData(std::array<float, waveformSize>& inputMin,
    std::array<float, waveformSize>& inputMax,
    std::array<float, waveformSize>& clippedMin,
    std::array<float, waveformSize>& clippedMax,
    std::array<float, waveformSize>& removedMin,
    std::array<float, waveformSize>& removedMax) const
{
    const int writeIndex = waveformWriteIndex.load(std::memory_order_acquire) % waveformSize;

    for (int i = 0; i < waveformSize; ++i)
    {
        const int sourceIndex = (writeIndex + i) % waveformSize;

        inputMin[(size_t)i] = waveformInputMin[(size_t)sourceIndex].load(std::memory_order_relaxed);
        inputMax[(size_t)i] = waveformInputMax[(size_t)sourceIndex].load(std::memory_order_relaxed);
        clippedMin[(size_t)i] = waveformClippedMin[(size_t)sourceIndex].load(std::memory_order_relaxed);
        clippedMax[(size_t)i] = waveformClippedMax[(size_t)sourceIndex].load(std::memory_order_relaxed);
        removedMin[(size_t)i] = waveformRemovedMin[(size_t)sourceIndex].load(std::memory_order_relaxed);
        removedMax[(size_t)i] = waveformRemovedMax[(size_t)sourceIndex].load(std::memory_order_relaxed);
    }
}

float JPClipperAudioProcessor::estimateOutputTruePeak(const juce::AudioBuffer<float>& buffer)
{
    float truePeak = 0.0f;
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const int stateChannel = juce::jlimit(0, maxDcChannels - 1, ch);
        const auto* data = buffer.getReadPointer(ch);
        const float previousBlockLastSample = truePeakPreviousOutput[(size_t)stateChannel];

        for (int i = 0; i < numSamples; ++i)
            truePeak = juce::jmax(truePeak, std::abs(data[i]));

        for (int i = 0; i < numSamples - 1; ++i)
        {
            const float p0 = i == 0 ? previousBlockLastSample : data[i - 1];
            const float p1 = data[i];
            const float p2 = data[i + 1];
            const float p3 = i + 2 < numSamples ? data[i + 2] : p2;

            truePeak = juce::jmax(truePeak, std::abs(catmullRom(p0, p1, p2, p3, 0.25f)));
            truePeak = juce::jmax(truePeak, std::abs(catmullRom(p0, p1, p2, p3, 0.50f)));
            truePeak = juce::jmax(truePeak, std::abs(catmullRom(p0, p1, p2, p3, 0.75f)));
        }

        if (numSamples > 0)
            truePeakPreviousOutput[(size_t)stateChannel] = data[numSamples - 1];
    }

    return truePeak;
}

void JPClipperAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    const float inputGainDb = apvts.getRawParameterValue("inputGain")->load();
    const float thresholdDb = apvts.getRawParameterValue("threshold")->load();
    const float outputGainDb = apvts.getRawParameterValue("outputGain")->load();
    const float ceilingDb = apvts.getRawParameterValue("ceiling")->load();
    const float mixTarget = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("mix")->load() / 100.0f);
    const float softKneeTarget = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("softKnee")->load() / 100.0f);
    const float colorAmountTarget = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("color")->load() / 100.0f);
    const float stereoLinkTarget = juce::jlimit(0.0f, 1.0f, apvts.getRawParameterValue("stereoLink")->load() / 100.0f);
    const float scopeZoom = apvts.getRawParameterValue("scopeZoom")->load();
    const bool autoGainEnabled = apvts.getRawParameterValue("autoGain")->load() > 0.5f;
    const bool dcBlockerEnabled = apvts.getRawParameterValue("dcBlocker")->load() > 0.5f;
    const bool deltaListenEnabled = apvts.getRawParameterValue("deltaListen")->load() > 0.5f;
    const bool bypassEnabled = apvts.getRawParameterValue("bypass")->load() > 0.5f;
    const bool matchEnabled = apvts.getRawParameterValue("match")->load() > 0.5f;
    const int oversamplingChoice = (int)apvts.getRawParameterValue("oversampling")->load();
    const int phaseModeChoice = (int)apvts.getRawParameterValue("phaseMode")->load();
    const int mode = (int)apvts.getRawParameterValue("mode")->load();
    const int processMode = (int)apvts.getRawParameterValue("processMode")->load();
    const int liftMode = (int)apvts.getRawParameterValue("liftMode")->load();

    updateOversamplingIfNeeded(oversamplingChoice, phaseModeChoice);
    waveformSamplesPerPointCurrent = waveformSamplesPerPointBase * juce::jlimit(1, 8, (int)std::round(scopeZoom));

    const float inputGainTarget = juce::Decibels::decibelsToGain(inputGainDb);
    const float thresholdTarget = juce::jmax(0.00001f, juce::Decibels::decibelsToGain(thresholdDb));
    const float outputGainTarget = juce::Decibels::decibelsToGain(outputGainDb);
    const float ceilingGainTarget = juce::jmax(0.00001f, juce::Decibels::decibelsToGain(ceilingDb));
    const float autoGainTarget = autoGainEnabled
        ? juce::Decibels::decibelsToGain(-inputGainDb)
        : 1.0f;
    const float liftAmountTarget = getLiftAmountForMode(liftMode);
    const float bypassTarget = bypassEnabled ? 1.0f : 0.0f;

    fillParameterSmoothingBuffers(numSamples,
        inputGainTarget,
        thresholdTarget,
        autoGainTarget,
        outputGainTarget,
        ceilingGainTarget,
        mixTarget,
        softKneeTarget,
        liftAmountTarget,
        colorAmountTarget,
        stereoLinkTarget,
        bypassTarget);

    blockMaxInput = 0.0f;
    blockMaxOutput = 0.0f;
    blockMaxClipping = 0.0f;
    blockMaxReductionDb = 0.0f;

    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer, true);

    float blockInputLeft = 0.0f;
    float blockInputRight = 0.0f;
    double inputEnergy = 0.0;
    int inputSamples = 0;

    for (int ch = 0; ch < dryBuffer.getNumChannels(); ++ch)
    {
        const auto* data = dryBuffer.getReadPointer(ch);

        for (int i = 0; i < dryBuffer.getNumSamples(); ++i)
        {
            const float x = data[i] * inputGainValues[(size_t)i];
            const float absX = std::abs(x);

            if (ch == 0)
                blockInputLeft = juce::jmax(blockInputLeft, absX);
            else if (ch == 1)
                blockInputRight = juce::jmax(blockInputRight, absX);

            inputEnergy += (double)x * (double)x;
            ++inputSamples;
        }
    }

    if (dryBuffer.getNumChannels() == 1)
        blockInputRight = blockInputLeft;

    if (processMode != processStereo && buffer.getNumChannels() >= 2)
        encodeStereoToMidSide(buffer);

    if (oversampling != nullptr)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        auto oversampledBlock = oversampling->processSamplesUp(block);

        const size_t oversampledSamples = oversampledBlock.getNumSamples();
        const double originalToOversampledRatio = oversampledSamples > 0
            ? (double)numSamples / (double)oversampledSamples
            : 1.0;

        for (size_t i = 0; i < oversampledSamples; ++i)
        {
            const int sourceSample = numSamples > 0
                ? juce::jlimit(0, numSamples - 1, (int)std::floor((double)i * originalToOversampledRatio))
                : 0;

            const size_t sourceIndex = (size_t)sourceSample;
            const float inputGain = inputGainValues[sourceIndex];
            const float threshold = thresholdValues[sourceIndex];
            const float liftAmount = liftAmountValues[sourceIndex];
            const float colorAmount = colorAmountValues[sourceIndex];
            const float autoGain = autoGainValues[sourceIndex];
            const float softKnee = softKneeValues[sourceIndex];
            const float stereoLink = stereoLinkValues[sourceIndex];

            float maxLiftedAbs = 0.0f;

            if (stereoLink > 0.0001f && oversampledBlock.getNumChannels() > 1)
            {
                for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
                {
                    if (!shouldProcessDomainChannel((int)ch, (int)oversampledBlock.getNumChannels(), processMode))
                        continue;

                    auto* data = oversampledBlock.getChannelPointer(ch);
                    const float x = data[i] * inputGain;
                    const float lifted = processLiftSample(x, threshold, liftAmount);
                    maxLiftedAbs = juce::jmax(maxLiftedAbs, std::abs(lifted));
                }
            }

            float linkedGain = 1.0f;

            if (maxLiftedAbs > 1.0e-8f)
            {
                float linkedClipDelta = 0.0f;
                const float linkedClippedAbs = std::abs(processClipperSample(maxLiftedAbs, threshold, softKnee, mode, linkedClipDelta));
                linkedGain = juce::jlimit(0.0f, 1.0f, linkedClippedAbs / maxLiftedAbs);
            }

            for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
            {
                auto* data = oversampledBlock.getChannelPointer(ch);

                if (!shouldProcessDomainChannel((int)ch, (int)oversampledBlock.getNumChannels(), processMode))
                    continue;

                const float x = data[i] * inputGain;
                const float lifted = processLiftSample(x, threshold, liftAmount);

                blockMaxInput = juce::jmax(blockMaxInput, std::abs(x));

                float clipDelta = 0.0f;
                const float independentClipped = processClipperSample(lifted, threshold, softKnee, mode, clipDelta);
                const float linkedClipped = lifted * linkedGain;
                const float clipped = independentClipped + (linkedClipped - independentClipped) * stereoLink;
                clipDelta = std::abs(lifted - clipped);
                const float coloured = processColorSample(clipped, threshold, colorAmount);
                updateClipReductionMetrics(lifted, clipped, clipDelta);

                const float wetOutput = coloured * autoGain;
                data[i] = deltaListenEnabled ? (lifted - wetOutput) : wetOutput;
            }
        }

        oversampling->processSamplesDown(block);

        // Waveform collection at original project rate, so the display speed remains stable.
        for (int i = 0; i < dryBuffer.getNumSamples(); ++i)
        {
            const size_t index = (size_t)i;
            const float dry = dryBuffer.getSample(0, i);
            const float x = dry * inputGainValues[index];
            const float lifted = processLiftSample(x, thresholdValues[index], liftAmountValues[index]);

            float clipDelta = 0.0f;
            const float clipped = processClipperSample(lifted, thresholdValues[index], softKneeValues[index], mode, clipDelta);
            const float coloured = processColorSample(clipped, thresholdValues[index], colorAmountValues[index]);

            pushWaveformSample(lifted, clipped, coloured);
        }
    }
    else
    {
        processWetBuffer(buffer,
            inputGainValues,
            thresholdValues,
            liftAmountValues,
            colorAmountValues,
            autoGainValues,
            softKneeValues,
            stereoLinkValues,
            mode,
            true,
            deltaListenEnabled,
            processMode);
    }

    if (processMode != processStereo && buffer.getNumChannels() >= 2)
        decodeMidSideToStereo(buffer);

    float blockOutputLeft = 0.0f;
    float blockOutputRight = 0.0f;
    double preMatchOutputEnergy = 0.0;
    int preMatchOutputSamples = 0;
    double outputEnergy = 0.0;
    int outputSamples = 0;

    if (deltaListenEnabled || bypassEnabled || !matchEnabled)
        matchTargetGain = 1.0f;

    matchGainSmoothed.setTargetValue(juce::jlimit(0.25119f, 3.98107f, matchTargetGain));

    for (int i = 0; i < numSamples; ++i)
        matchGainValues[(size_t)i] = matchGainSmoothed.getNextValue();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wetData = buffer.getWritePointer(ch);
        const auto* dryData = dryBuffer.getReadPointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const size_t index = (size_t)i;
            const float latencyCompensatedDry = getLatencyCompensatedDrySample(ch, dryData[i]);

            float processed = 0.0f;

            if (deltaListenEnabled)
            {
                processed = wetData[i];
            }
            else
            {
                const float wet = wetData[i];
                const float mix = mixValues[index];
                const float dryGain = 1.0f - mix;
                const float wetGain = mix;

                processed = latencyCompensatedDry * dryGain + wet * wetGain;
                processed *= outputGainValues[index];

                if (dcBlockerEnabled)
                    processed = processDcBlocker(processed, ch);

                preMatchOutputEnergy += (double)processed * (double)processed;
                ++preMatchOutputSamples;

                if (matchEnabled)
                    processed *= matchGainValues[index];

                const float ceilingGain = ceilingGainValues[index];
                processed = juce::jlimit(-ceilingGain, ceilingGain, processed);
            }

            const float bypassAmount = bypassValues[index];
            const float y = processed * (1.0f - bypassAmount) + latencyCompensatedDry * bypassAmount;

            wetData[i] = y;

            const float absY = std::abs(y);
            blockMaxOutput = juce::jmax(blockMaxOutput, absY);

            if (ch == 0)
                blockOutputLeft = juce::jmax(blockOutputLeft, absY);
            else if (ch == 1)
                blockOutputRight = juce::jmax(blockOutputRight, absY);

            outputEnergy += (double)y * (double)y;
            ++outputSamples;
        }
    }

    if (buffer.getNumChannels() == 1)
        blockOutputRight = blockOutputLeft;

    const float inputRms = inputSamples > 0
        ? (float)std::sqrt(inputEnergy / (double)inputSamples)
        : 0.0f;

    const float outputRms = outputSamples > 0
        ? (float)std::sqrt(outputEnergy / (double)outputSamples)
        : 0.0f;

    const float preMatchOutputRms = preMatchOutputSamples > 0
        ? (float)std::sqrt(preMatchOutputEnergy / (double)preMatchOutputSamples)
        : 0.0f;

    const float truePeakLevel = estimateOutputTruePeak(buffer);
    const float truePeakDb = juce::Decibels::gainToDecibels(truePeakLevel, -120.0f);
    const bool ispActive = truePeakDb >= ispAlertThresholdDb;
    const float finalBypassAmount = numSamples > 0 ? bypassValues[(size_t)numSamples - 1] : bypassTarget;
    const float processingVisibility = 1.0f - juce::jlimit(0.0f, 1.0f, finalBypassAmount);

    if (matchEnabled && !deltaListenEnabled && processingVisibility > 0.5f
        && inputRms > 1.0e-5f && preMatchOutputRms > 1.0e-5f)
    {
        const float targetDb = juce::jlimit(-12.0f, 12.0f,
            juce::Decibels::gainToDecibels(inputRms / preMatchOutputRms));

        matchTargetGain = juce::Decibels::decibelsToGain(targetDb);
    }
    else
    {
        matchTargetGain = 1.0f;
    }

    inputLevel.store(blockMaxInput);
    outputLevel.store(blockMaxOutput);
    clipAmount.store(blockMaxClipping * processingVisibility);
    clipReductionDb.store(blockMaxReductionDb * processingVisibility);

    inputLeftLevel.store(blockInputLeft);
    inputRightLevel.store(blockInputRight);
    outputLeftLevel.store(blockOutputLeft);
    outputRightLevel.store(blockOutputRight);

    const double blockMeanSquare = outputSamples > 0
        ? outputEnergy / (double)outputSamples
        : 0.0;

    const double shortTermWindowSamples = juce::jmax(1.0, currentSampleRate * 3.0);
    const double shortTermCoeff = std::exp(-(double)numSamples / shortTermWindowSamples);
    shortTermLufsMeanSquare = shortTermLufsMeanSquare * shortTermCoeff
        + blockMeanSquare * (1.0 - shortTermCoeff);

    inputRmsDb.store(juce::Decibels::gainToDecibels(inputRms, -60.0f));
    outputRmsDb.store(juce::Decibels::gainToDecibels(outputRms, -60.0f));
    outputShortTermLufs.store(meanSquareToApproxShortTermLufs(shortTermLufsMeanSquare));
    outputTruePeakDb.store(truePeakDb);
    matchGainDb.store(matchEnabled && !deltaListenEnabled && processingVisibility > 0.5f && numSamples > 0
        ? juce::Decibels::gainToDecibels(matchGainValues[(size_t)numSamples - 1], -60.0f)
        : 0.0f);
    outputIspActive.store(ispActive);
}


void JPClipperAudioProcessor::setEditorSize(int width, int height)
{
    const int clampedWidth = juce::jlimit(minEditorWidth, maxEditorWidth, width);
    const int clampedHeight = juce::jlimit(minEditorHeight, maxEditorHeight, height);

    editorWidth.store(clampedWidth);
    editorHeight.store(clampedHeight);
    apvts.state.setProperty(editorWidthProperty, clampedWidth, nullptr);
    apvts.state.setProperty(editorHeightProperty, clampedHeight, nullptr);
}

juce::File JPClipperAudioProcessor::getUserPresetDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("JPClipper")
        .getChildFile("User Presets");
}

juce::String JPClipperAudioProcessor::sanitisePresetFileName(const juce::String& presetName)
{
    auto safeName = presetName.trim();

    const juce::String invalidCharacters = "\\/:*?\"<>|";
    for (int i = 0; i < invalidCharacters.length(); ++i)
        safeName = safeName.replaceCharacter(invalidCharacters[i], '_');

    safeName = safeName.removeCharacters("\r\n\t").trim();

    while (safeName.contains("  "))
        safeName = safeName.replace("  ", " ");

    return safeName.isNotEmpty() ? safeName : "Untitled";
}

void JPClipperAudioProcessor::refreshUserPresets()
{
    userPresets.clear();

    auto presetDirectory = getUserPresetDirectory();

    if (!presetDirectory.exists())
        return;

    auto files = presetDirectory.findChildFiles(juce::File::findFiles, false, "*" + juce::String(userPresetExtension));

    for (const auto& file : files)
    {
        const auto name = file.getFileNameWithoutExtension().trim().toUpperCase();

        if (name.isNotEmpty())
            userPresets.push_back({ name, file });
    }
}

int JPClipperAudioProcessor::getNumUserPresets() const
{
    return (int)userPresets.size();
}

juce::String JPClipperAudioProcessor::getUserPresetName(int index) const
{
    if (!juce::isPositiveAndBelow(index, (int)userPresets.size()))
        return {};

    return userPresets[(size_t)index].name;
}

bool JPClipperAudioProcessor::loadPresetState(const juce::ValueTree& presetState,
    const juce::String& presetName,
    const juce::String& undoName)
{
    if (!presetState.isValid())
        return false;

    auto state = presetState.createCopy();

    if (auto abTree = state.getChildWithName(abCompareTreeName); abTree.isValid())
        state.removeChild(abTree, nullptr);

    const int loadedVersion = (int)state.getProperty(stateVersionProperty, 1);
    migrateState(state, loadedVersion);
    state.setProperty(currentPresetNameProperty, presetName, nullptr);
    state.setProperty(editorWidthProperty, editorWidth.load(), nullptr);
    state.setProperty(editorHeightProperty, editorHeight.load(), nullptr);

    undoManager.beginNewTransaction(undoName);
    apvts.replaceState(state);
    currentPresetName = presetName;
    resetParameterSmoothers();
    storeCurrentStateToABSlot(activeABSlot.load());

    return true;
}

bool JPClipperAudioProcessor::loadUserPreset(int index)
{
    if (!juce::isPositiveAndBelow(index, (int)userPresets.size()))
        return false;

    const auto preset = userPresets[(size_t)index];

    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(preset.file));

    if (xml == nullptr || !xml->hasTagName(apvts.state.getType()))
        return false;

    auto presetState = juce::ValueTree::fromXml(*xml);
    return loadPresetState(presetState, preset.name, "Load User Preset: " + preset.name);
}

bool JPClipperAudioProcessor::saveUserPreset(const juce::String& presetName)
{
    const auto cleanName = presetName.trim().toUpperCase();

    if (cleanName.isEmpty())
        return false;

    auto presetDirectory = getUserPresetDirectory();

    if (!presetDirectory.exists() && !presetDirectory.createDirectory())
        return false;

    const auto safeFileName = sanitisePresetFileName(cleanName) + juce::String(userPresetExtension);
    auto presetFile = presetDirectory.getChildFile(safeFileName);

    auto state = createCleanStateForStorage();
    state.setProperty(currentPresetNameProperty, cleanName, nullptr);
    state.setProperty(editorWidthProperty, editorWidth.load(), nullptr);
    state.setProperty(editorHeightProperty, editorHeight.load(), nullptr);
    state.setProperty("presetType", "user", nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    if (xml == nullptr || !xml->writeTo(presetFile))
        return false;

    currentPresetName = cleanName;
    refreshUserPresets();
    storeCurrentStateToABSlot(activeABSlot.load());

    return true;
}

bool JPClipperAudioProcessor::deleteUserPreset(int index)
{
    if (!juce::isPositiveAndBelow(index, (int)userPresets.size()))
        return false;

    const auto preset = userPresets[(size_t)index];
    const bool deleted = preset.file.deleteFile();

    if (!deleted)
        return false;

    if (currentPresetName == preset.name)
        currentPresetName = "SESSION";

    refreshUserPresets();
    return true;
}

juce::String JPClipperAudioProcessor::createClipboardSettingsText()
{
    auto state = createCleanStateForStorage();
    state.setProperty("clipboardFormat", "JPClipperSettings", nullptr);

    if (auto xml = state.createXml())
        return xml->toString();

    return {};
}

bool JPClipperAudioProcessor::loadClipboardSettingsText(const juce::String& text)
{
    if (text.trim().isEmpty())
        return false;

    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(text));

    if (xml == nullptr || !xml->hasTagName(apvts.state.getType()))
        return false;

    auto state = juce::ValueTree::fromXml(*xml);

    if (!state.isValid())
        return false;

    return loadPresetState(state, "PASTED SETTINGS", "Paste Plugin Settings");
}

int JPClipperAudioProcessor::getNumFactoryPresets() const
{
    return (int)factoryPresets.size();
}

juce::String JPClipperAudioProcessor::getFactoryPresetName(int index) const
{
    if (!juce::isPositiveAndBelow(index, (int)factoryPresets.size()))
        return {};

    return factoryPresets[(size_t)index].name;
}

juce::String JPClipperAudioProcessor::getCurrentPresetName() const
{
    return currentPresetName;
}

void JPClipperAudioProcessor::setParameterRawValue(const juce::String& parameterID, float rawValue, bool notifyHost)
{
    if (auto* parameter = apvts.getParameter(parameterID))
    {
        if (notifyHost)
            parameter->beginChangeGesture();

        parameter->setValueNotifyingHost(parameter->convertTo0to1(rawValue));

        if (notifyHost)
            parameter->endChangeGesture();
    }
}

void JPClipperAudioProcessor::loadFactoryPreset(int index)
{
    if (!juce::isPositiveAndBelow(index, (int)factoryPresets.size()))
        return;

    const auto& preset = factoryPresets[(size_t)index];

    undoManager.beginNewTransaction("Load Preset: " + juce::String(preset.name));

    setParameterRawValue("inputGain", preset.inputGainDb, true);
    setParameterRawValue("threshold", preset.thresholdDb, true);
    setParameterRawValue("outputGain", preset.outputGainDb, true);
    setParameterRawValue("ceiling", preset.ceilingDb, true);
    setParameterRawValue("autoGain", preset.autoGain ? 1.0f : 0.0f, true);
    setParameterRawValue("match", preset.match ? 1.0f : 0.0f, true);
    setParameterRawValue("mix", preset.mixPercent, true);
    setParameterRawValue("dcBlocker", preset.dcBlocker ? 1.0f : 0.0f, true);
    setParameterRawValue("deltaListen", preset.deltaListen ? 1.0f : 0.0f, true);
    setParameterRawValue("bypass", preset.bypass ? 1.0f : 0.0f, true);
    setParameterRawValue("oversampling", (float)preset.oversampling, true);
    setParameterRawValue("phaseMode", (float)preset.phaseMode, true);
    setParameterRawValue("mode", (float)preset.mode, true);
    setParameterRawValue("processMode", (float)preset.processMode, true);
    setParameterRawValue("softKnee", preset.softKneePercent, true);
    setParameterRawValue("liftMode", (float)preset.liftMode, true);
    setParameterRawValue("color", preset.colorAmountPercent, true);
    setParameterRawValue("stereoLink", preset.stereoLinkPercent, true);
    setParameterRawValue("scopeMode", (float)preset.scopeMode, true);
    setParameterRawValue("scopeZoom", preset.scopeZoom, true);

    currentPresetName = preset.name;
    storeCurrentStateToABSlot(activeABSlot.load());
}

juce::ValueTree JPClipperAudioProcessor::createCleanStateForStorage()
{
    auto state = apvts.copyState();

    if (auto abTree = state.getChildWithName(abCompareTreeName); abTree.isValid())
        state.removeChild(abTree, nullptr);

    state.setProperty(stateVersionProperty, currentStateVersion, nullptr);
    state.setProperty(currentPresetNameProperty, currentPresetName, nullptr);
    state.setProperty(editorWidthProperty, editorWidth.load(), nullptr);
    state.setProperty(editorHeightProperty, editorHeight.load(), nullptr);

    return state;
}

void JPClipperAudioProcessor::initialiseABSlots()
{
    const juce::ScopedLock lock(abStateLock);
    const auto state = createCleanStateForStorage();

    abSlotStates[0] = state.createCopy();
    abSlotStates[1] = state.createCopy();
    activeABSlot.store(0);
}

void JPClipperAudioProcessor::storeCurrentStateToABSlot(int slot)
{
    slot = juce::jlimit(0, 1, slot);

    const juce::ScopedLock lock(abStateLock);
    abSlotStates[slot] = createCleanStateForStorage();
}

void JPClipperAudioProcessor::recallABSlot(int slot)
{
    slot = juce::jlimit(0, 1, slot);

    juce::ValueTree stateToRecall;

    {
        const juce::ScopedLock lock(abStateLock);

        if (!abSlotStates[slot].isValid())
            abSlotStates[slot] = createCleanStateForStorage();

        stateToRecall = abSlotStates[slot].createCopy();
        activeABSlot.store(slot);
        currentPresetName = slot == 0 ? "COMPARE A" : "COMPARE B";
    }

    if (stateToRecall.isValid())
    {
        if (auto abTree = stateToRecall.getChildWithName(abCompareTreeName); abTree.isValid())
            stateToRecall.removeChild(abTree, nullptr);

        apvts.replaceState(stateToRecall);
        resetParameterSmoothers();
    }
}

void JPClipperAudioProcessor::selectABSlot(int slot)
{
    slot = juce::jlimit(0, 1, slot);
    const int previousSlot = activeABSlot.load();

    if (previousSlot == slot)
    {
        storeCurrentStateToABSlot(slot);
        return;
    }

    storeCurrentStateToABSlot(previousSlot);
    recallABSlot(slot);
}

void JPClipperAudioProcessor::migrateState(juce::ValueTree& state, int loadedVersion) const
{
    auto ensureProperty = [&state](const juce::Identifier& property, const juce::var& defaultValue)
        {
            if (!state.hasProperty(property))
                state.setProperty(property, defaultValue, nullptr);
        };

    if (loadedVersion < 2)
    {
        ensureProperty("softKnee", 50.0f);
        ensureProperty("scopeMode", 1);
    }

    if (loadedVersion < 3)
    {
        ensureProperty(currentPresetNameProperty, "SESSION");
    }

    if (loadedVersion < 4)
    {
        ensureProperty(editorWidthProperty, defaultEditorWidth);
        ensureProperty(editorHeightProperty, defaultEditorHeight);
    }

    if (loadedVersion < 5)
        ensureProperty("liftMode", 0);

    if (loadedVersion < 7)
        ensureProperty("color", 0.0f);

    if (loadedVersion < 8)
        ensureProperty("match", false);

    if (loadedVersion < 9)
        ensureProperty("scopeZoom", 3.0f);

    if (loadedVersion < 10)
        ensureProperty("stereoLink", 100.0f);

    if (loadedVersion < 6
        && (int)state.getProperty(editorWidthProperty, defaultEditorWidth) == 1080
        && (int)state.getProperty(editorHeightProperty, defaultEditorHeight) == defaultEditorHeight)
    {
        state.setProperty(editorWidthProperty, defaultEditorWidth, nullptr);
        state.setProperty(editorHeightProperty, defaultEditorHeight, nullptr);
    }

    state.setProperty(editorWidthProperty,
        juce::jlimit(minEditorWidth, maxEditorWidth, (int)state.getProperty(editorWidthProperty, defaultEditorWidth)),
        nullptr);
    state.setProperty(editorHeightProperty,
        juce::jlimit(minEditorHeight, maxEditorHeight, (int)state.getProperty(editorHeightProperty, defaultEditorHeight)),
        nullptr);
    state.setProperty(stateVersionProperty, currentStateVersion, nullptr);
}

void JPClipperAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = createCleanStateForStorage();

    {
        const juce::ScopedLock lock(abStateLock);
        const int activeSlot = juce::jlimit(0, 1, activeABSlot.load());
        abSlotStates[activeSlot] = state.createCopy();

        juce::ValueTree abTree(abCompareTreeName);
        abTree.setProperty(activeSlotProperty, activeSlot, nullptr);

        for (int slot = 0; slot < 2; ++slot)
        {
            auto slotState = abSlotStates[slot].isValid()
                ? abSlotStates[slot].createCopy()
                : state.createCopy();

            if (auto nestedAB = slotState.getChildWithName(abCompareTreeName); nestedAB.isValid())
                slotState.removeChild(nestedAB, nullptr);

            slotState.setProperty("slotIndex", slot, nullptr);
            abTree.addChild(slotState, -1, nullptr);
        }

        state.addChild(abTree, -1, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void JPClipperAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState == nullptr || !xmlState->hasTagName(apvts.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml(*xmlState);
    const int loadedVersion = (int)state.getProperty(stateVersionProperty, 1);
    const auto loadedPresetName = state.getProperty(currentPresetNameProperty, "SESSION").toString();

    auto abTree = state.getChildWithName(abCompareTreeName);

    if (abTree.isValid())
        state.removeChild(abTree, nullptr);

    migrateState(state, loadedVersion);

    editorWidth.store(juce::jlimit(minEditorWidth, maxEditorWidth, (int)state.getProperty(editorWidthProperty, defaultEditorWidth)));
    editorHeight.store(juce::jlimit(minEditorHeight, maxEditorHeight, (int)state.getProperty(editorHeightProperty, defaultEditorHeight)));
    refreshUserPresets();

    {
        const juce::ScopedLock lock(abStateLock);

        if (abTree.isValid() && abTree.getNumChildren() >= 2)
        {
            abSlotStates[0] = abTree.getChild(0).createCopy();
            abSlotStates[1] = abTree.getChild(1).createCopy();
            activeABSlot.store(juce::jlimit(0, 1, (int)abTree.getProperty(activeSlotProperty, 0)));
        }
        else
        {
            abSlotStates[0] = state.createCopy();
            abSlotStates[1] = state.createCopy();
            activeABSlot.store(0);
        }

        currentPresetName = loadedPresetName.isNotEmpty() ? loadedPresetName : "SESSION";
    }

    apvts.replaceState(state);
    resetParameterSmoothers();
}


juce::AudioProcessorEditor* JPClipperAudioProcessor::createEditor()
{
    return new JPClipperAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JPClipperAudioProcessor();
}
