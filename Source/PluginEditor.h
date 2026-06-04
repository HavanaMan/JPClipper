#pragma once
#include <JuceHeader.h>
#include <array>
#include "PluginProcessor.h"

#include "UI/UI.h"

class WaveformDisplay : public juce::Component,
    public juce::SettableTooltipClient
{
public:
    void setData(const std::array<float, JPClipperAudioProcessor::waveformSize>& inputMin,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& inputMax,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& clippedMin,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& clippedMax,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& removedMin,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& removedMax,
        float thresholdLinear,
        float clipReductionDbValue,
        float outputRmsDbValue,
        float outputShortTermLufsValue,
        float outputTruePeakDbValue,
        float ceilingDbValue,
        bool deltaModeEnabled,
        int scopeModeValue,
        int processModeValue)
    {
        inputMinData = inputMin;
        inputMaxData = inputMax;
        clippedMinData = clippedMin;
        clippedMaxData = clippedMax;
        removedMinData = removedMin;
        removedMaxData = removedMax;
        threshold = thresholdLinear;
        clipReductionDb = clipReductionDbValue;
        outputRmsDb = outputRmsDbValue;
        outputShortTermLufs = outputShortTermLufsValue;
        outputTruePeakDb = outputTruePeakDbValue;
        ceilingDb = ceilingDbValue;
        deltaMode = deltaModeEnabled;
        scopeMode = juce::jlimit(0, 3, scopeModeValue);
        processMode = juce::jlimit(0, 3, processModeValue);
        repaint();
    }

    void paint(juce::Graphics& g) override;

private:
    void drawAudioWaveform(juce::Graphics& g,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& minData,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& maxData,
        juce::Rectangle<float> area,
        juce::Colour colour,
        float displayRange);

    void drawReductionWaveform(juce::Graphics& g,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& minData,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& maxData,
        juce::Rectangle<float> area,
        juce::Colour colour);

    void drawClipAmountHistory(juce::Graphics& g,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& minData,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& maxData,
        juce::Rectangle<float> area,
        juce::Colour colour,
        float currentReductionDb);

    void drawClipCrests(juce::Graphics& g,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& inputMin,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& inputMax,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& clippedMin,
        const std::array<float, JPClipperAudioProcessor::waveformSize>& clippedMax,
        juce::Rectangle<float> area,
        juce::Colour colour,
        float displayRange);

    std::array<float, JPClipperAudioProcessor::waveformSize> inputMinData{};
    std::array<float, JPClipperAudioProcessor::waveformSize> inputMaxData{};
    std::array<float, JPClipperAudioProcessor::waveformSize> clippedMinData{};
    std::array<float, JPClipperAudioProcessor::waveformSize> clippedMaxData{};
    std::array<float, JPClipperAudioProcessor::waveformSize> removedMinData{};
    std::array<float, JPClipperAudioProcessor::waveformSize> removedMaxData{};

    float threshold = 1.0f;
    float clipReductionDb = 0.0f;
    float outputRmsDb = -60.0f;
    float outputShortTermLufs = -60.0f;
    float outputTruePeakDb = -60.0f;
    float ceilingDb = 0.0f;
    bool deltaMode = false;
    int scopeMode = 0;
    int processMode = 0;
};


class JPClipperAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    explicit JPClipperAudioProcessorEditor(JPClipperAudioProcessor&);
    ~JPClipperAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    void setupSlider(juce::Slider& slider);
    void setupLabel(juce::Label& label, const juce::String& text);
    void setupToggleButton(juce::TextButton& button);
    void setupSmallButton(juce::TextButton& button);

    void setOversamplingChoice(int choiceIndex);
    void updateOversamplingButtons();
    void setModeChoice(int choiceIndex);
    void updateModeButtons();
    void setProcessingModeChoice(int choiceIndex);
    void updateProcessingModeBox();
    void setPhaseChoice(int choiceIndex);
    void updatePhaseButtons();
    void setLiftChoice(int choiceIndex);
    void updateLiftButtons();
    void setScopeChoice(int choiceIndex);
    void updateScopeButtons();
    void updateABButtons();
    void updateUtilityButtons();
    void refreshPresetBox();
    void updatePresetButtons();
    void loadSelectedPreset();
    void showSavePresetDialog();
    void deleteSelectedUserPreset();
    int getSelectedUserPresetIndex() const;
    void checkLatestVersionAsync();
    void showUpdateAvailableBadge(const juce::String& latestVersion, const juce::String& updateUrl);
    void setThemeVariant(int newVariant);
    void updateThemeButtons();

    float getUiScale() const;
    juce::AffineTransform getContentTransform() const;
    void applyGlobalScaleTransform();

    void drawPanel(juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title);
    void drawHeader(juce::Graphics& g, juce::Rectangle<int> area);

    JPClipperAudioProcessor& audioProcessor;

    AnalogKnobLookAndFeel analogKnobLookAndFeel;
    HardwareButtonLookAndFeel hardwareButtonLookAndFeel;
    HeaderIconButtonLookAndFeel headerIconButtonLookAndFeel;
    BlueComboBoxLookAndFeel blueComboBoxLookAndFeel;
    HardwareToggleButtonLookAndFeel hardwareToggleButtonLookAndFeel;
    JPOverlayLookAndFeel jpOverlayLookAndFeel;
    juce::TooltipWindow tooltipWindow{ this, 700 };

    HardwareMeter inputMeter;
    HardwareMeter outputMeter;
    WaveformDisplay waveformDisplay;

    juce::ComboBox presetBox;
    juce::ComboBox oversamplingBox;
    juce::ComboBox processingModeBox;
    juce::ComboBox scopeTypeBox;
    juce::ComboBox scopeZoomBox;
    juce::HyperlinkButton footerLink;
    juce::Label versionLabel;
    juce::TextButton updateAvailableButton;
    juce::ComboBox themeBox;
    juce::TextButton savePresetButton;
    juce::TextButton deletePresetButton;
    juce::TextButton aButton;
    juce::TextButton bButton;
    juce::TextButton copyABButton;
    juce::TextButton pasteSettingsButton;
    juce::TextButton undoButton;
    juce::TextButton redoButton;
    juce::TextButton helpButton;

    juce::Slider inputSlider;
    juce::Slider thresholdSlider;
    juce::Slider kneeSlider;
    juce::Slider outputSlider;
    juce::Slider ceilingSlider;
    juce::Slider mixSlider;
    juce::Slider colorSlider;
    juce::Slider stereoLinkSlider;
    juce::Slider scopeZoomSlider;
    juce::Slider spacerSlider;
    juce::Label spacerLabel;
    juce::Label spacerValueLabel;

    juce::TextButton autoGainButton;
    juce::TextButton matchButton;
    juce::TextButton dcBlockerButton;
    juce::TextButton deltaButton;
    juce::TextButton bypassButton;

    juce::TextButton hardButton;
    juce::TextButton softButton;
    juce::TextButton linearPhaseButton;
    juce::TextButton minPhaseButton;
    juce::Slider liftSlider;

    juce::TextButton osOffButton;
    juce::TextButton os2Button;
    juce::TextButton os4Button;
    juce::TextButton os8Button;
    juce::TextButton os16Button;
    juce::TextButton os32Button;

    juce::TextButton scopeOutButton;
    juce::TextButton scopeInOutButton;
    juce::TextButton scopeDeltaButton;
    juce::TextButton scopePeaksButton;

    juce::Label inputLabel;
    juce::Label thresholdLabel;
    juce::Label kneeLabel;
    juce::Label outputLabel;
    juce::Label ceilingLabel;
    juce::Label mixLabel;
    juce::Label colorLabel;
    juce::Label autoGainLabel;
    juce::Label matchLabel;
    juce::Label matchAmountLabel;
    juce::Label dcBlockerLabel;
    juce::Label deltaLabel;
    juce::Label bypassLabel;
    juce::Label modeLabel;
    juce::Label processingModeLabel;
    juce::Label liftLabel;
    juce::Label liftValueLabel;
    juce::Label oversamplingLabel;
    juce::Label phaseLabel;
    juce::Label scopeLabel;
    juce::Label inputValueLabel;
    juce::Label thresholdValueLabel;
    juce::Label kneeValueLabel;
    juce::Label outputValueLabel;
    juce::Label ceilingValueLabel;
    juce::Label mixValueLabel;
    juce::Label colorValueLabel;
    juce::Label stereoLinkLabel;
    juce::Label stereoLinkValueLabel;
    juce::Label scopeZoomLabel;
    juce::Label scopeZoomValueLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<Attachment> inputAttachment;
    std::unique_ptr<Attachment> thresholdAttachment;
    std::unique_ptr<Attachment> kneeAttachment;
    std::unique_ptr<Attachment> outputAttachment;
    std::unique_ptr<Attachment> ceilingAttachment;
    std::unique_ptr<Attachment> mixAttachment;
    std::unique_ptr<Attachment> colorAttachment;
    std::unique_ptr<Attachment> stereoLinkAttachment;
    std::unique_ptr<Attachment> scopeZoomAttachment;

    std::unique_ptr<ButtonAttachment> autoGainAttachment;
    std::unique_ptr<ButtonAttachment> matchAttachment;
    std::unique_ptr<ButtonAttachment> dcBlockerAttachment;
    std::unique_ptr<ButtonAttachment> deltaAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    float inputLeftMeterSmoothed = 0.0f;
    float inputRightMeterSmoothed = 0.0f;
    float outputLeftMeterSmoothed = 0.0f;
    float outputRightMeterSmoothed = 0.0f;
    float inputRmsDbSmoothed = -60.0f;
    float outputRmsDbSmoothed = -60.0f;
    float clipReductionDbSmoothed = 0.0f;
    int currentThemeVariant = JPTheme::blue;
    bool ignorePresetBoxChange = false;
    bool tooltipsEnabled = false;
    juce::String latestAvailableVersion;
    juce::String latestVersionUrl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JPClipperAudioProcessorEditor)
};
