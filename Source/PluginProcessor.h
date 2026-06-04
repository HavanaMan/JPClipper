#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

class JPClipperAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int waveformSize = 512;

    JPClipperAudioProcessor();
    ~JPClipperAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    using juce::AudioProcessor::processBlock;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "JPClipper"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::UndoManager& getUndoManager() { return undoManager; }
    const juce::UndoManager& getUndoManager() const { return undoManager; }

    int getNumFactoryPresets() const;
    juce::String getFactoryPresetName(int index) const;
    void loadFactoryPreset(int index);

    void refreshUserPresets();
    int getNumUserPresets() const;
    juce::String getUserPresetName(int index) const;
    bool loadUserPreset(int index);
    bool saveUserPreset(const juce::String& presetName);
    bool deleteUserPreset(int index);
    juce::File getUserPresetDirectory() const;

    juce::String getCurrentPresetName() const;

    int getEditorWidth() const { return editorWidth.load(); }
    int getEditorHeight() const { return editorHeight.load(); }
    void setEditorSize(int width, int height);

    juce::String createClipboardSettingsText();
    bool loadClipboardSettingsText(const juce::String& text);

    void selectABSlot(int slot);
    int getActiveABSlot() const { return activeABSlot.load(); }

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

    float getInputLevel() const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }
    float getClipAmount() const { return clipAmount.load(); }
    float getClipReductionDb() const { return clipReductionDb.load(); }
    float getInputLeftLevel() const { return inputLeftLevel.load(); }
    float getInputRightLevel() const { return inputRightLevel.load(); }
    float getOutputLeftLevel() const { return outputLeftLevel.load(); }
    float getOutputRightLevel() const { return outputRightLevel.load(); }
    float getInputRmsDb() const { return inputRmsDb.load(); }
    float getOutputRmsDb() const { return outputRmsDb.load(); }
    float getOutputTruePeakDb() const { return outputTruePeakDb.load(); }
    bool getOutputIspActive() const { return outputIspActive.load(); }
    float getMatchGainDb() const { return matchGainDb.load(); }
    float getInputLufs() const { return getInputRmsDb(); }
    float getOutputLufs() const { return getOutputShortTermLufs(); }
    float getOutputShortTermLufs() const { return outputShortTermLufs.load(); }

    void copyWaveformData(std::array<float, waveformSize>& inputMin,
        std::array<float, waveformSize>& inputMax,
        std::array<float, waveformSize>& clippedMin,
        std::array<float, waveformSize>& clippedMax,
        std::array<float, waveformSize>& removedMin,
        std::array<float, waveformSize>& removedMax) const;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    float processClipperSample(float inputSample,
        float threshold,
        float softKnee,
        int mode,
        float& clipDelta) const;

    float processLiftSample(float inputSample,
        float threshold,
        float liftAmount) const;

    float processColorSample(float clippedSample,
        float threshold,
        float colorAmount) const;

    void processWetBuffer(juce::AudioBuffer<float>& buffer,
        const std::vector<float>& inputGainValues,
        const std::vector<float>& thresholdValues,
        const std::vector<float>& liftAmountValues,
        const std::vector<float>& colorAmountValues,
        const std::vector<float>& autoGainValues,
        const std::vector<float>& softKneeValues,
        const std::vector<float>& stereoLinkValues,
        int mode,
        bool collectWaveform,
        bool deltaListen,
        int processMode);

    void pushWaveformSample(float inputSample, float clippedSample, float outputSample);
    void resetWaveform();
    void resetParameterSmoothers();
    void resizeParameterSmoothingBuffers(int numSamples);
    void fillParameterSmoothingBuffers(int numSamples,
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
        float bypassTarget);
    void updateClipReductionMetrics(float inputSample, float clippedSample, float clipDelta);
    float estimateOutputTruePeak(const juce::AudioBuffer<float>& buffer);

    void setParameterRawValue(const juce::String& parameterID, float rawValue, bool notifyHost);
    juce::ValueTree createCleanStateForStorage();
    bool loadPresetState(const juce::ValueTree& presetState, const juce::String& presetName, const juce::String& undoName);
    static juce::String sanitisePresetFileName(const juce::String& presetName);
    void initialiseABSlots();
    void storeCurrentStateToABSlot(int slot);
    void recallABSlot(int slot);
    void migrateState(juce::ValueTree& state, int loadedVersion) const;

    void rebuildOversamplingEngines();
    int getOversamplingEngineIndex(int oversamplingChoice, int phaseModeChoice) const;
    void updateOversamplingIfNeeded(int oversamplingChoice, int phaseModeChoice);
    void resizeDryLatencyBuffer();
    void resetDryLatencyBuffer();
    float getLatencyCompensatedDrySample(int channel, float inputSample);
    void resetDcBlocker();
    float processDcBlocker(float x, int channel);

    std::atomic<float> inputLevel{ 0.0f };
    std::atomic<float> outputLevel{ 0.0f };
    std::atomic<float> clipAmount{ 0.0f };
    std::atomic<float> clipReductionDb{ 0.0f };
    std::atomic<float> inputLeftLevel{ 0.0f };
    std::atomic<float> inputRightLevel{ 0.0f };
    std::atomic<float> outputLeftLevel{ 0.0f };
    std::atomic<float> outputRightLevel{ 0.0f };
    std::atomic<float> inputRmsDb{ -60.0f };
    std::atomic<float> outputRmsDb{ -60.0f };
    std::atomic<float> outputTruePeakDb{ -60.0f };
    std::atomic<float> outputShortTermLufs{ -60.0f };
    std::atomic<float> matchGainDb{ 0.0f };
    std::atomic<bool> outputIspActive{ false };

    static constexpr double parameterSmoothingTimeSeconds = 0.020;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inputGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> thresholdSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> autoGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> ceilingGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> softKneeSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> liftAmountSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> colorAmountSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> stereoLinkSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> matchGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bypassSmoothed;

    std::vector<float> inputGainValues;
    std::vector<float> thresholdValues;
    std::vector<float> autoGainValues;
    std::vector<float> outputGainValues;
    std::vector<float> ceilingGainValues;
    std::vector<float> mixValues;
    std::vector<float> softKneeValues;
    std::vector<float> liftAmountValues;
    std::vector<float> colorAmountValues;
    std::vector<float> stereoLinkValues;
    std::vector<float> matchGainValues;
    std::vector<float> bypassValues;

    std::array<std::atomic<float>, waveformSize> waveformInputMin{};
    std::array<std::atomic<float>, waveformSize> waveformInputMax{};
    std::array<std::atomic<float>, waveformSize> waveformClippedMin{};
    std::array<std::atomic<float>, waveformSize> waveformClippedMax{};
    std::array<std::atomic<float>, waveformSize> waveformRemovedMin{};
    std::array<std::atomic<float>, waveformSize> waveformRemovedMax{};

    std::atomic<int> waveformWriteIndex{ 0 };

    float waveformInputBlockMin = 0.0f;
    float waveformInputBlockMax = 0.0f;
    float waveformClippedBlockMin = 0.0f;
    float waveformClippedBlockMax = 0.0f;
    float waveformRemovedBlockMin = 0.0f;
    float waveformRemovedBlockMax = 0.0f;

    int waveformBlockCounter = 0;

    // Lower = more detailed / faster display movement.
    // Higher = wider time overview / slower display movement.
    static constexpr int waveformSamplesPerPointBase = 64;
    int waveformSamplesPerPointCurrent = waveformSamplesPerPointBase * 3;

    // Oversampling: 0=Off, 1=2x, 2=4x, 3=8x, 4=16x, 5=32x.
    // Engines are built in prepareToPlay() and switched by pointer in processBlock(),
    // avoiding allocation/filter initialisation on the realtime thread when the user changes mode.
    static constexpr int oversamplingChoiceCount = 6;
    static constexpr int phaseModeChoiceCount = 2;
    static constexpr int oversamplingEngineCount = oversamplingChoiceCount * phaseModeChoiceCount;

    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, oversamplingEngineCount> oversamplingEngines;
    juce::dsp::Oversampling<float>* oversampling = nullptr;

    int currentOversamplingChoice = -1;
    int currentPhaseModeChoice = -1;
    int currentOversamplingLatencySamples = 0;
    int maxOversamplingLatencySamples = 0;
    double currentSampleRate = 44100.0;
    int currentMaxBlockSize = 512;
    int currentNumChannels = 2;

    std::vector<std::vector<float>> dryLatencyBuffer;
    std::vector<int> dryLatencyWritePositions;

    // Simple one-pole DC blocker state.
    static constexpr int maxDcChannels = 16;
    std::array<float, maxDcChannels> dcX1{};
    std::array<float, maxDcChannels> dcY1{};
    float dcR = 0.995f;

    std::array<float, maxDcChannels> truePeakPreviousOutput{};

    float blockMaxInput = 0.0f;
    float blockMaxOutput = 0.0f;
    float blockMaxClipping = 0.0f;
    float blockMaxReductionDb = 0.0f;
    float matchTargetGain = 1.0f;
    double shortTermLufsMeanSquare = 0.0;

    juce::CriticalSection abStateLock;
    juce::ValueTree abSlotStates[2];
    std::atomic<int> activeABSlot{ 0 };
    juce::String currentPresetName{ "DEFAULT" };

    struct UserPreset
    {
        juce::String name;
        juce::File file;
    };

    std::vector<UserPreset> userPresets;

    std::atomic<int> editorWidth{ 1020 };
    std::atomic<int> editorHeight{ 600 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JPClipperAudioProcessor)
};
