#pragma once
#include <JuceHeader.h>
#include <array>

class JPTheme
{
public:
    enum Variant
    {
        blue = 0,
        black = 1,
        greenTheme = 2,
        pink = 3,
        yellow = 4,
        purple = 5,
        orange = 6
    };

    static int& variantStorage()
    {
        static int currentVariant = blue;
        return currentVariant;
    }

    static void setVariant(int newVariant)
    {
        variantStorage() = juce::jlimit(0, 6, newVariant);
    }

    static int getVariant()
    {
        return variantStorage();
    }

    static juce::Colour bg()
    {
        // Main template/body colour. Keep this lighter than band() so the
        // waveform/footer area stays the darker visual anchor in every theme.
        switch (getVariant())
        {
        case black:      return juce::Colour(42, 43, 49);
        case greenTheme: return juce::Colour(12, 104, 76);
        case pink:       return juce::Colour(128, 50, 96);
        case yellow:     return juce::Colour(156, 119, 25);
        case purple:     return juce::Colour(92, 60, 150);
        case orange:     return juce::Colour(176, 82, 25);
        default:         return juce::Colour(6, 92, 124);
        }
    }

    static juce::Colour panel()
    {
        switch (getVariant())
        {
        case black:      return juce::Colour(30, 31, 36);
        case greenTheme: return juce::Colour(10, 82, 61);
        case pink:       return juce::Colour(104, 39, 80);
        case yellow:     return juce::Colour(128, 94, 18);
        case purple:     return juce::Colour(72, 44, 124);
        case orange:     return juce::Colour(142, 62, 17);
        default:         return juce::Colour(18, 19, 29);
        }
    }

    static juce::Colour panel2()
    {
        switch (getVariant())
        {
        case black:      return juce::Colour(36, 37, 43);
        case greenTheme: return juce::Colour(14, 98, 72);
        case pink:       return juce::Colour(122, 47, 94);
        case yellow:     return juce::Colour(144, 106, 22);
        case purple:     return juce::Colour(84, 52, 144);
        case orange:     return juce::Colour(160, 72, 20);
        default:         return juce::Colour(25, 26, 39);
        }
    }

    static juce::Colour band()
    {
        switch (getVariant())
        {
        case black:      return juce::Colour(10, 11, 14);
        case greenTheme: return juce::Colour(4, 55, 42);
        case pink:       return juce::Colour(72, 24, 55);
        case yellow:     return juce::Colour(78, 58, 11);
        case purple:     return juce::Colour(42, 27, 84);
        case orange:     return juce::Colour(89, 35, 9);
        default:         return juce::Colour(2, 58, 88);
        }
    }

    static juce::Colour line()
    {
        switch (getVariant())
        {
        case black:      return juce::Colour(82, 84, 94);
        case greenTheme: return juce::Colour(64, 148, 112);
        case pink:       return juce::Colour(164, 78, 132);
        case yellow:     return juce::Colour(197, 153, 45);
        case purple:     return juce::Colour(136, 108, 214);
        case orange:     return juce::Colour(220, 125, 54);
        default:         return juce::Colour(58, 61, 82);
        }
    }

    static juce::Colour text() { return juce::Colour(238, 236, 247); }
    static juce::Colour dimText() { return juce::Colour(178, 184, 194); }

    static juce::Colour accent()
    {
        switch (getVariant())
        {
        case black:      return juce::Colour(206, 212, 222);
        case greenTheme: return juce::Colour(106, 238, 170);
        case pink:       return juce::Colour(255, 142, 207);
        case yellow:     return juce::Colour(255, 214, 86);
        case purple:     return juce::Colour(190, 154, 255);
        case orange:     return juce::Colour(255, 151, 73);
        default:         return juce::Colour(83, 201, 255);
        }
    }

    static juce::Colour cyan() { return accent(); }

    static juce::Colour green()
    {
        switch (getVariant())
        {
        case black:      return juce::Colour(198, 205, 214);
        case greenTheme: return juce::Colour(102, 242, 168);
        case pink:       return juce::Colour(255, 150, 210);
        case yellow:     return juce::Colour(255, 224, 100);
        case purple:     return juce::Colour(200, 168, 255);
        case orange:     return juce::Colour(255, 168, 92);
        default:         return juce::Colour(88, 224, 154);
        }
    }

    static juce::Colour red() { return juce::Colour(255, 86, 118); }

    static juce::Colour accentDark()
    {
        switch (getVariant())
        {
        case black:      return juce::Colour(55, 57, 64);
        case greenTheme: return juce::Colour(12, 125, 88);
        case pink:       return juce::Colour(166, 58, 122);
        case yellow:     return juce::Colour(185, 138, 24);
        case purple:     return juce::Colour(116, 78, 190);
        case orange:     return juce::Colour(202, 85, 20);
        default:         return juce::Colour(5, 103, 143);
        }
    }

    static juce::Colour accentDeep()
    {
        switch (getVariant())
        {
        case black:      return juce::Colour(18, 19, 23);
        case greenTheme: return juce::Colour(5, 72, 54);
        case pink:       return juce::Colour(92, 31, 70);
        case yellow:     return juce::Colour(99, 73, 13);
        case purple:     return juce::Colour(58, 38, 110);
        case orange:     return juce::Colour(106, 42, 10);
        default:         return juce::Colour(3, 74, 107);
        }
    }

    static juce::Colour themeDotColour(int variant)
    {
        switch (juce::jlimit(0, 6, variant))
        {
        case black:      return juce::Colour(35, 36, 42);
        case greenTheme: return juce::Colour(88, 224, 154);
        case pink:       return juce::Colour(255, 132, 199);
        case yellow:     return juce::Colour(255, 214, 86);
        case purple:     return juce::Colour(190, 154, 255);
        case orange:     return juce::Colour(255, 151, 73);
        default:         return juce::Colour(83, 201, 255);
        }
    }

    static juce::String variantName(int variant)
    {
        switch (juce::jlimit(0, 6, variant))
        {
        case black:      return "BLACK";
        case greenTheme: return "GREEN";
        case pink:       return "PINK";
        case yellow:     return "YELLOW";
        case purple:     return "PURPLE";
        case orange:     return "ORANGE";
        default:         return "BLUE";
        }
    }

    static juce::Font labelFont(float size = 12.0f)
    {
        return juce::Font(juce::FontOptions("Bahnschrift", size, juce::Font::plain));
    }

    static juce::Font titleFont(float size = 28.0f)
    {
        return juce::Font(juce::FontOptions("Bahnschrift", size, juce::Font::bold));
    }
};

class AnalogKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AnalogKnobLookAndFeel();

    void drawRotarySlider(juce::Graphics& g,
        int x,
        int y,
        int width,
        int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider& slider) override;

private:
    juce::Colour pointer{ 238, 236, 247 };
    juce::Colour accent{ 83, 201, 255 };
};


class HardwareButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics& g,
        juce::Button& button,
        const juce::Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g,
        juce::TextButton& button,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;
};

class HeaderIconButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics& g,
        juce::Button& button,
        const juce::Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g,
        juce::TextButton& button,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;
};

class BlueComboBoxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
        int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
        bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
        const juce::String& text, const juce::String& shortcutKeyText,
        const juce::Drawable* icon, const juce::Colour* textColour) override;
};


class JPOverlayLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override;
    juce::Rectangle<int> getTooltipBounds(const juce::String& tipText,
        juce::Point<int> screenPos,
        juce::Rectangle<int> parentArea) override;

    void drawButtonBackground(juce::Graphics& g,
        juce::Button& button,
        const juce::Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g,
        juce::TextButton& button,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;
};


class HardwareToggleButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics& g,
        juce::Button& button,
        const juce::Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g,
        juce::TextButton& button,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;
};

class ThemeDotButton : public juce::Button
{
public:
    explicit ThemeDotButton(const juce::String& buttonName = {})
        : juce::Button(buttonName)
    {
    }

    void setThemeVariant(int newVariant)
    {
        themeVariant = juce::jlimit(0, 6, newVariant);
        repaint();
    }

    int getThemeVariant() const { return themeVariant; }

    void setSelected(bool shouldBeSelected)
    {
        selected = shouldBeSelected;
        repaint();
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto b = getLocalBounds().toFloat().reduced(2.0f);
        const auto colour = JPTheme::themeDotColour(themeVariant);

        g.setColour(juce::Colours::black.withAlpha(0.20f));
        g.fillEllipse(b.translated(0.0f, 1.5f));

        g.setColour(colour.withAlpha(shouldDrawButtonAsDown ? 0.78f : 1.0f));
        g.fillEllipse(b.reduced(shouldDrawButtonAsHighlighted ? 0.0f : 1.0f));

        g.setColour(juce::Colours::white.withAlpha(selected ? 0.88f : 0.28f));
        g.drawEllipse(b.reduced(selected ? 0.0f : 1.5f), selected ? 2.0f : 1.0f);

        if (selected)
        {
            g.setColour(colour.withAlpha(0.22f));
            g.drawEllipse(b.expanded(2.0f), 1.4f);
        }
    }

private:
    int themeVariant = JPTheme::blue;
    bool selected = false;
};

class HardwareMeter : public juce::Component,
    public juce::SettableTooltipClient
{
public:
    void setLevels(float newLeftLevel,
        float newRightLevel,
        float newRmsDbValue,
        float newTruePeakDbValue = -60.0f,
        bool newIspActive = false)
    {
        leftLevel = juce::jlimit(0.0f, 1.0f, newLeftLevel);
        rightLevel = juce::jlimit(0.0f, 1.0f, newRightLevel);
        rmsDbValue = newRmsDbValue;
        truePeakDbValue = newTruePeakDbValue;
        ispActive = newIspActive;
        leftPeakHold = juce::jmax(leftLevel, leftPeakHold * 0.965f);
        rightPeakHold = juce::jmax(rightLevel, rightPeakHold * 0.965f);
        truePeakHoldDbValue = juce::jmax(truePeakHoldDbValue, truePeakDbValue);
        repaint();
    }

    void setTitle(const juce::String& newTitle)
    {
        title = newTitle;
        repaint();
    }

    void resetPeakHold()
    {
        leftPeakHold = leftLevel;
        rightPeakHold = rightLevel;
        truePeakHoldDbValue = truePeakDbValue;
        repaint();
    }

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent&) override { resetPeakHold(); }

private:
    float leftLevel = 0.0f;
    float rightLevel = 0.0f;
    float leftPeakHold = 0.0f;
    float rightPeakHold = 0.0f;
    float rmsDbValue = -60.0f;
    float truePeakDbValue = -60.0f;
    float truePeakHoldDbValue = -60.0f;
    bool ispActive = false;
    juce::String title = "INPUT";
};


// Reusable building blocks for future Juicy Plugins. They are intentionally
// header-only so a new plugin can include this file without adding another
// translation unit to the project.
class JPAnalogKnobControl : public juce::Component
{
public:
    explicit JPAnalogKnobControl(const juce::String& knobName)
    {
        title.setText(knobName, juce::dontSendNotification);
        title.setJustificationType(juce::Justification::centred);
        title.setFont(JPTheme::titleFont(12.0f));
        title.setColour(juce::Label::textColourId, JPTheme::accent().brighter(0.55f));
        addAndMakeVisible(title);

        slider.setName(knobName);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel(&lookAndFeel);
        addAndMakeVisible(slider);
    }

    ~JPAnalogKnobControl() override
    {
        slider.setLookAndFeel(nullptr);
    }

    juce::Slider& getSlider() { return slider; }
    juce::Label& getLabel() { return title; }

    void setLarge(bool shouldBeLarge)
    {
        large = shouldBeLarge;
        resized();
    }

    void refreshTheme()
    {
        title.setColour(juce::Label::textColourId, JPTheme::accent().brighter(0.55f));
        repaint();
    }

    void resized() override
    {
        auto area = getLocalBounds();
        title.setBounds(area.removeFromTop(large ? 20 : 18));
        slider.setBounds(area.reduced(0, 2));
    }

private:
    bool large = false;
    AnalogKnobLookAndFeel lookAndFeel;
    juce::Label title;
    juce::Slider slider;
};

class JPBrandHeaderComponent : public juce::Component
{
public:
    void setTexts(const juce::String& brand, const juce::String& product)
    {
        brandText = brand;
        productText = product;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        auto logo = area.removeFromLeft(220.0f).reduced(10.0f, 6.0f);
        g.setColour(JPTheme::accent().brighter(0.60f));
        g.setFont(JPTheme::titleFont(12.5f));
        g.drawText(brandText, logo.removeFromTop(18.0f), juce::Justification::centredLeft, false);
        g.setFont(JPTheme::titleFont(27.0f));
        g.drawText(productText, logo, juce::Justification::centredLeft, false);
    }

private:
    juce::String brandText { "JUICY PLUGINS" };
    juce::String productText { "JP|PLUGIN" };
};

class JPFooterComponent : public juce::Component
{
public:
    JPFooterComponent()
        : website("JUICYPADS.COM", juce::URL("https://www.juicypads.com"))
    {
        addAndMakeVisible(website);
        addAndMakeVisible(themeBox);
        addAndMakeVisible(versionLabel);

        themeBox.addItem("BLUE", JPTheme::blue + 1);
        themeBox.addItem("BLACK", JPTheme::black + 1);
        themeBox.addItem("GREEN", JPTheme::greenTheme + 1);
        themeBox.addItem("PINK", JPTheme::pink + 1);
        themeBox.addItem("YELLOW", JPTheme::yellow + 1);
        themeBox.addItem("PURPLE", JPTheme::purple + 1);
        themeBox.addItem("ORANGE", JPTheme::orange + 1);
        themeBox.setSelectedId(JPTheme::getVariant() + 1, juce::dontSendNotification);
        themeBox.setLookAndFeel(&comboLookAndFeel);

        website.setColour(juce::HyperlinkButton::textColourId, JPTheme::accent().brighter(0.65f));
        versionLabel.setJustificationType(juce::Justification::centredRight);
        versionLabel.setFont(JPTheme::titleFont(12.0f));
        refreshTheme();
    }

    ~JPFooterComponent() override
    {
        themeBox.setLookAndFeel(nullptr);
    }

    juce::ComboBox& getThemeBox() { return themeBox; }

    void setVersionText(const juce::String& text)
    {
        versionLabel.setText(text, juce::dontSendNotification);
    }

    void setUpdateAvailable(bool available)
    {
        updateAvailable = available;
        refreshTheme();
    }

    void refreshTheme()
    {
        website.setColour(juce::HyperlinkButton::textColourId, JPTheme::accent().brighter(0.65f));
        versionLabel.setColour(juce::Label::textColourId, updateAvailable ? JPTheme::red() : JPTheme::accent().brighter(0.65f));
        themeBox.setColour(juce::ComboBox::backgroundColourId, JPTheme::accentDeep().withAlpha(0.78f));
        themeBox.setColour(juce::ComboBox::outlineColourId, JPTheme::accent().brighter(0.45f).withAlpha(0.48f));
        themeBox.setColour(juce::ComboBox::textColourId, JPTheme::accent().brighter(0.65f));
        themeBox.setColour(juce::ComboBox::arrowColourId, JPTheme::accent().brighter(0.55f));
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(JPTheme::band());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(24, 0);
        website.setBounds(area.removeFromLeft(180));
        versionLabel.setBounds(area.removeFromRight(120));
        const int themeW = 112;
        themeBox.setBounds(getLocalBounds().withSizeKeepingCentre(themeW, 24));
    }

private:
    bool updateAvailable = false;
    BlueComboBoxLookAndFeel comboLookAndFeel;
    juce::HyperlinkButton website;
    juce::ComboBox themeBox;
    juce::Label versionLabel;
};

// Inline implementations for the shared Juicy UI look-and-feels and meters.
inline AnalogKnobLookAndFeel::AnalogKnobLookAndFeel()
{
    setColour(juce::Slider::rotarySliderFillColourId, JPTheme::accent());
    setColour(juce::Slider::rotarySliderOutlineColourId, JPTheme::line());
    setColour(juce::Slider::thumbColourId, pointer);
}

inline void AnalogKnobLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(3.0f);
    const float size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto knobBounds = juce::Rectangle<float>(0.0f, 0.0f, size, size).withCentre(bounds.getCentre());

    const bool mainGainKnob = slider.getName().equalsIgnoreCase("INPUT") || slider.getName().equalsIgnoreCase("OUTPUT")
        || slider.getName().equalsIgnoreCase("IN") || slider.getName().equalsIgnoreCase("OUT");
    const float cx = knobBounds.getCentreX();
    const float cy = knobBounds.getCentreY();
    const float radius = size * 0.5f;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    g.setColour(juce::Colour(0, 51, 76).withAlpha(0.28f));
    g.fillEllipse(knobBounds.translated(0.0f, 4.0f).expanded(1.5f));

    juce::ColourGradient face(JPTheme::accentDark().brighter(0.08f), cx, knobBounds.getY(),
        JPTheme::accentDeep().darker(0.15f), cx, knobBounds.getBottom(), false);
    face.addColour(0.55, JPTheme::accentDark());
    g.setGradientFill(face);
    g.fillEllipse(knobBounds);

    juce::ColourGradient highlight(juce::Colour(193, 238, 255).withAlpha(0.22f),
        cx, knobBounds.getY(),
        juce::Colours::transparentWhite,
        cx, knobBounds.getCentreY(),
        false);
    g.setGradientFill(highlight);
    g.fillEllipse(knobBounds.reduced(4.0f).withTrimmedBottom(size * 0.40f));

    g.setColour(JPTheme::accentDeep().withAlpha(0.68f));
    g.drawEllipse(knobBounds.reduced(3.0f), 3.0f);

    juce::Path track;
    track.addCentredArc(cx, cy, radius * 0.78f, radius * 0.78f, 0.0f,
        rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(JPTheme::accentDark().brighter(0.30f).withAlpha(0.64f));
    g.strokePath(track, juce::PathStrokeType(mainGainKnob ? 7.4f : 6.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc(cx, cy, radius * 0.78f, radius * 0.78f, 0.0f,
        rotaryStartAngle, angle, true);
    g.setColour(JPTheme::accent().brighter(0.55f).interpolatedWith(JPTheme::cyan(), sliderPosProportional * 0.35f));
    g.strokePath(valueArc, juce::PathStrokeType(mainGainKnob ? 7.8f : 6.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto getDisplayText = [&slider]() -> juce::String
        {
            const auto name = slider.getName();
            const double v = slider.getValue();

            if (name.equalsIgnoreCase("INPUT") || name.equalsIgnoreCase("OUTPUT")
                || name.equalsIgnoreCase("IN") || name.equalsIgnoreCase("OUT")
                || name.equalsIgnoreCase("THRESHOLD") || name.equalsIgnoreCase("CEILING"))
                return juce::String::formatted("%+.1f", v);

            if (name.equalsIgnoreCase("KNEE") || name.equalsIgnoreCase("MIX")
                || name.equalsIgnoreCase("COLOR") || name.equalsIgnoreCase("ST LINK"))
                return juce::String::formatted("%.0f%%", v);

            if (name.equalsIgnoreCase("LIFT"))
            {
                static const juce::StringArray names{ "OFF", "BODY", "DENS", "PUSH" };
                return names[juce::jlimit(0, names.size() - 1, (int)std::round(v))];
            }

            if (name.equalsIgnoreCase("SCOPE TYPE"))
            {
                static const juce::StringArray names{ "OUT", "IN/OUT", "DELTA", "PEAKS" };
                return names[juce::jlimit(0, names.size() - 1, (int)std::round(v))];
            }

            if (name.equalsIgnoreCase("SCOPE X"))
                return juce::String::formatted("%.0fX", v);

            return juce::String();
        };

    const auto valueText = getDisplayText();
    if (valueText.isNotEmpty())
    {
        g.setFont(JPTheme::titleFont(juce::jlimit(8.2f, 13.4f, size * (mainGainKnob ? 0.165f : 0.150f))));
        g.setColour(juce::Colour(220, 248, 255).withAlpha(0.94f));
        g.drawFittedText(valueText, knobBounds.reduced(size * 0.18f).toNearestInt(),
            juce::Justification::centred, 1, 0.82f);
    }
}


inline void HardwareButtonLookAndFeel::drawButtonBackground(juce::Graphics& g,
    juce::Button& button,
    const juce::Colour&,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    auto outer = button.getLocalBounds().toFloat().reduced(0.5f);
    const bool on = button.getToggleState();
    const bool isUpdateBadge = button.getName().equalsIgnoreCase("update");

    const float radius = outer.getHeight() * 0.5f;
    const auto fixedUpdateGreen = juce::Colour(88, 224, 154);
    const auto fill = on && isUpdateBadge
        ? fixedUpdateGreen.withAlpha(shouldDrawButtonAsDown ? 0.40f : 0.30f)
        : on
        ? JPTheme::accent().brighter(0.35f).withAlpha(shouldDrawButtonAsDown ? 0.34f : 0.25f)
        : JPTheme::accentDeep().withAlpha(shouldDrawButtonAsHighlighted ? 0.42f : 0.30f);

    g.setColour(fill);
    g.fillRoundedRectangle(outer, radius);

    if (shouldDrawButtonAsHighlighted)
    {
        g.setColour(JPTheme::accent().brighter(0.55f).withAlpha(0.10f));
        g.fillRoundedRectangle(outer.reduced(1.0f), radius);
    }

    g.setColour(on && isUpdateBadge ? fixedUpdateGreen.withAlpha(0.86f)
        : on ? JPTheme::accent().brighter(0.55f).withAlpha(0.72f)
        : JPTheme::accentDeep().withAlpha(0.62f));
    g.drawRoundedRectangle(outer.reduced(0.5f), radius, 1.4f);
}

inline void HardwareButtonLookAndFeel::drawButtonText(juce::Graphics& g,
    juce::TextButton& button,
    bool,
    bool)
{
    const bool on = button.getToggleState();
    const bool isUpdateBadge = button.getName().equalsIgnoreCase("update");

    g.setFont(JPTheme::titleFont(isUpdateBadge ? 11.2f : 13.2f));
    g.setColour(on && isUpdateBadge ? juce::Colour(218, 255, 236)
        : on ? JPTheme::accent().brighter(0.70f)
        : JPTheme::accent().brighter(0.38f));

    auto textArea = button.getLocalBounds().reduced(isUpdateBadge ? 8 : 4, 0);
    if (isUpdateBadge)
        textArea = textArea.translated(0, -1);

    g.drawText(button.getButtonText(),
        textArea,
        juce::Justification::centred,
        false);
}

inline void HeaderIconButtonLookAndFeel::drawButtonBackground(juce::Graphics& g,
    juce::Button& button,
    const juce::Colour&,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    auto area = button.getLocalBounds().toFloat();
    const float size = juce::jmin(area.getWidth(), area.getHeight()) - 1.5f;
    auto b = juce::Rectangle<float>(size, size).withCentre(area.getCentre());
    const bool isBypass = button.getName().equalsIgnoreCase("bypass");

    // For the power button the parameter stores BYPASS, so toggleState == true
    // means the processor is OFF. Visually we display the opposite: green
    // when the plugin is processing, warm red when it is bypassed.
    const bool pluginIsProcessing = isBypass ? !button.getToggleState() : button.getToggleState();

    juce::Colour top;
    juce::Colour bottom;
    juce::Colour outline;

    if (isBypass)
    {
        if (pluginIsProcessing)
        {
            top = juce::Colour(88, 224, 154).withAlpha(shouldDrawButtonAsHighlighted ? 0.70f : 0.54f);
            bottom = juce::Colour(16, 103, 70).withAlpha(0.80f);
            outline = juce::Colour(166, 255, 211).withAlpha(shouldDrawButtonAsHighlighted ? 0.96f : 0.78f);
        }
        else
        {
            top = juce::Colour(255, 112, 120).withAlpha(shouldDrawButtonAsHighlighted ? 0.68f : 0.52f);
            bottom = juce::Colour(118, 28, 50).withAlpha(0.78f);
            outline = juce::Colour(255, 188, 190).withAlpha(shouldDrawButtonAsHighlighted ? 0.98f : 0.82f);
        }
    }
    else
    {
        top = pluginIsProcessing ? JPTheme::accent().brighter(0.55f).withAlpha(0.24f)
                                 : JPTheme::accentDeep().withAlpha(shouldDrawButtonAsHighlighted ? 0.50f : 0.30f);
        bottom = top;
        outline = JPTheme::accent().brighter(0.45f).withAlpha(shouldDrawButtonAsHighlighted || pluginIsProcessing ? 0.74f : 0.42f);
    }

    g.setColour(top.interpolatedWith(bottom, 0.42f));
    g.fillEllipse(b);

    if (shouldDrawButtonAsDown)
    {
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.fillEllipse(b.reduced(1.4f));
    }

    if (isBypass)
    {
        g.setColour(juce::Colours::white.withAlpha(pluginIsProcessing ? 0.08f : 0.05f));
        g.fillEllipse(b.reduced(5.0f).withTrimmedBottom(b.getHeight() * 0.55f));
    }

    g.setColour(outline);
    g.drawEllipse(b.reduced(0.5f), isBypass ? 1.7f : 1.25f);
}

inline void HeaderIconButtonLookAndFeel::drawButtonText(juce::Graphics& g,
    juce::TextButton& button,
    bool,
    bool)
{
    auto area = button.getLocalBounds().toFloat();
    const auto icon = button.getName();
    const bool isBypass = icon.equalsIgnoreCase("bypass");
    const float iconSize = juce::jmin(area.getWidth(), area.getHeight()) * (isBypass ? 0.56f : 0.48f);
    auto b = juce::Rectangle<float>(iconSize, iconSize).withCentre(area.getCentre());
    const auto c = button.isEnabled()
        ? (isBypass
            ? (!button.getToggleState() ? juce::Colour(222, 255, 237) : juce::Colour(255, 226, 226))
            : JPTheme::accent().brighter(0.68f))
        : JPTheme::accent().withAlpha(0.36f);

    g.setColour(c);

    auto drawArrow = [&](bool redo)
        {
            auto arcBounds = b.reduced(0.5f);
            const float start = redo ? -2.65f : -0.50f;
            const float end = redo ? 0.70f : 2.85f;
            juce::Path arc;
            arc.addCentredArc(arcBounds.getCentreX(), arcBounds.getCentreY(),
                arcBounds.getWidth() * 0.42f, arcBounds.getHeight() * 0.42f,
                0.0f, start, end, true);
            g.strokePath(arc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            juce::Path head;
            if (redo)
            {
                head.startNewSubPath(b.getRight() - 2.0f, b.getCentreY() - 3.5f);
                head.lineTo(b.getRight() - 2.0f, b.getCentreY() + 3.5f);
                head.lineTo(b.getRight() + 3.5f, b.getCentreY());
            }
            else
            {
                head.startNewSubPath(b.getX() + 2.0f, b.getCentreY() - 3.5f);
                head.lineTo(b.getX() + 2.0f, b.getCentreY() + 3.5f);
                head.lineTo(b.getX() - 3.5f, b.getCentreY());
            }
            g.fillPath(head);
        };

    if (icon == "save")
    {
        auto disk = b.reduced(0.5f);
        g.drawRoundedRectangle(disk, 1.7f, 1.7f);
        g.fillRoundedRectangle(juce::Rectangle<float>(disk.getX() + 2.6f, disk.getY() + 2.2f,
            disk.getWidth() - 5.2f, disk.getHeight() * 0.25f), 0.9f);
        g.setColour(c.withAlpha(0.48f));
        g.fillRoundedRectangle(juce::Rectangle<float>(disk.getRight() - 5.2f, disk.getY() + 2.5f,
            2.2f, disk.getHeight() * 0.20f), 0.7f);
        g.setColour(c);
        auto label = juce::Rectangle<float>(disk.getX() + 3.4f, disk.getBottom() - 6.2f,
            disk.getWidth() - 6.8f, 3.8f);
        g.drawRoundedRectangle(label, 0.8f, 1.3f);
    }
    else if (icon == "delete")
    {
        auto can = b.reduced(1.0f, 0.5f);
        g.drawLine(can.getX() - 0.5f, can.getY() + 4.0f, can.getRight() + 0.5f, can.getY() + 4.0f, 1.8f);
        g.drawLine(can.getCentreX() - 3.0f, can.getY() + 1.0f, can.getCentreX() + 3.0f, can.getY() + 1.0f, 1.5f);

        juce::Path body;
        body.startNewSubPath(can.getX() + 1.5f, can.getY() + 6.0f);
        body.lineTo(can.getRight() - 1.5f, can.getY() + 6.0f);
        body.lineTo(can.getRight() - 3.0f, can.getBottom());
        body.lineTo(can.getX() + 3.0f, can.getBottom());
        body.closeSubPath();
        g.strokePath(body, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.drawLine(can.getCentreX() - 2.6f, can.getY() + 8.0f, can.getCentreX() - 1.7f, can.getBottom() - 2.2f, 1.1f);
        g.drawLine(can.getCentreX() + 2.6f, can.getY() + 8.0f, can.getCentreX() + 1.7f, can.getBottom() - 2.2f, 1.1f);
    }
    else if (icon == "copy")
    {
        auto back = b.translated(2.1f, -2.1f).reduced(2.0f);
        auto front = b.translated(-1.8f, 1.8f).reduced(2.0f);
        g.drawRoundedRectangle(back, 1.8f, 1.5f);
        g.drawRoundedRectangle(front, 1.8f, 1.7f);
    }
    else if (icon == "paste")
    {
        auto clip = b.reduced(2.8f, 1.8f);
        g.drawRoundedRectangle(clip.withTrimmedTop(3.8f), 1.8f, 1.7f);
        g.fillRoundedRectangle(juce::Rectangle<float>(clip.getCentreX() - 4.8f, clip.getY(), 9.6f, 4.8f), 1.6f);
        g.drawLine(clip.getX() + 4.5f, clip.getCentreY(), clip.getRight() - 4.5f, clip.getCentreY(), 1.3f);
        g.drawLine(clip.getX() + 4.5f, clip.getCentreY() + 4.8f, clip.getRight() - 6.0f, clip.getCentreY() + 4.8f, 1.3f);
    }
    else if (icon == "undo")
    {
        drawArrow(false);
    }
    else if (icon == "redo")
    {
        drawArrow(true);
    }
    else if (icon == "help")
    {
        g.setFont(JPTheme::titleFont(18.0f));
        g.setColour(button.getToggleState()
            ? JPTheme::accent().brighter(0.78f)
            : JPTheme::accent().brighter(0.68f));
        g.drawText("?",
            button.getLocalBounds().reduced(1, 0),
            juce::Justification::centred,
            false);
    }

    else if (icon == "bypass")
    {
        const bool pluginIsProcessing = !button.getToggleState();

        const auto powerColour = pluginIsProcessing
            ? juce::Colour(222, 255, 237)
            : juce::Colour(255, 226, 226);

        const auto glowColour = pluginIsProcessing
            ? juce::Colour(88, 224, 154)
            : juce::Colour(255, 86, 118);

        auto iconBox = button.getLocalBounds().toFloat();
        const float s = juce::jmin(iconBox.getWidth(), iconBox.getHeight()) * 0.58f;
        iconBox = juce::Rectangle<float>(s, s).withCentre(iconBox.getCentre().translated(0.0f, -1.35f));

        const float cx = iconBox.getCentreX();
        const float cy = iconBox.getCentreY();
        const float r = iconBox.getWidth() * 0.42f;
        const float stroke = juce::jlimit(1.7f, 2.35f, iconBox.getWidth() * 0.115f);

        juce::Path ring;
        ring.startNewSubPath(cx - r * 0.52f, cy - r * 0.43f);
        ring.cubicTo(cx - r * 1.02f, cy + r * 0.02f,
            cx - r * 0.88f, cy + r * 0.92f,
            cx, cy + r * 0.92f);
        ring.cubicTo(cx + r * 0.88f, cy + r * 0.92f,
            cx + r * 1.02f, cy + r * 0.02f,
            cx + r * 0.52f, cy - r * 0.43f);

        g.setColour(glowColour.withAlpha(pluginIsProcessing ? 0.22f : 0.28f));
        g.strokePath(ring, juce::PathStrokeType(stroke + 1.4f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

        g.setColour(powerColour);
        g.strokePath(ring, juce::PathStrokeType(stroke,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

        g.drawLine(cx, cy - r * 0.96f, cx, cy - r * 0.06f, stroke);

        g.setColour(powerColour.withAlpha(pluginIsProcessing ? 0.22f : 0.30f));
        g.fillEllipse(juce::Rectangle<float>(stroke * 1.10f, stroke * 1.10f)
            .withCentre({ cx, cy - r * 1.02f }));
    }
    else
    {
        g.setFont(JPTheme::titleFont(17.0f));
        g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
    }
}



inline void BlueComboBoxLookAndFeel::drawComboBox(juce::Graphics& g,
    int width,
    int height,
    bool isButtonDown,
    int,
    int,
    int,
    int,
    juce::ComboBox& box)
{
    auto b = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height).reduced(0.5f);
    const float radius = b.getHeight() * 0.5f;

    g.setColour(JPTheme::accentDark().withAlpha(isButtonDown ? 0.94f : 0.88f));
    g.fillRoundedRectangle(b, radius);

    g.setColour(JPTheme::accent().brighter(0.45f).withAlpha(box.hasKeyboardFocus(false) ? 0.72f : 0.42f));
    g.drawRoundedRectangle(b.reduced(0.5f), radius, 1.25f);

    auto arrowArea = b.removeFromRight(23.0f).reduced(6.0f, 0.0f);
    juce::Path arrow;
    arrow.startNewSubPath(arrowArea.getX(), arrowArea.getCentreY() - 2.0f);
    arrow.lineTo(arrowArea.getCentreX(), arrowArea.getCentreY() + 3.0f);
    arrow.lineTo(arrowArea.getRight(), arrowArea.getCentreY() - 2.0f);
    g.setColour(JPTheme::accent().brighter(0.65f).withAlpha(0.86f));
    g.strokePath(arrow, juce::PathStrokeType(1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

inline void BlueComboBoxLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(box.getLocalBounds().reduced(18, 1));
    label.setJustificationType(juce::Justification::centred);
    label.setFont(JPTheme::titleFont(box.getHeight() <= 24 ? 11.2f : 12.4f));
    label.setColour(juce::Label::textColourId, JPTheme::accent().brighter(0.65f));
    label.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    label.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label.setInterceptsMouseClicks(false, false);
}

inline void BlueComboBoxLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.fillAll(JPTheme::accentDark().darker(0.05f));

    g.setColour(JPTheme::accent().brighter(0.45f).withAlpha(0.30f));
    g.drawRect(juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f), 1.0f);
}

inline void BlueComboBoxLookAndFeel::drawPopupMenuItem(juce::Graphics& g,
    const juce::Rectangle<int>& area,
    bool isSeparator,
    bool isActive,
    bool isHighlighted,
    bool isTicked,
    bool,
    const juce::String& text,
    const juce::String&,
    const juce::Drawable*,
    const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour(JPTheme::accent().brighter(0.45f).withAlpha(0.22f));
        g.drawLine((float)area.getX() + 8.0f, (float)area.getCentreY(),
            (float)area.getRight() - 8.0f, (float)area.getCentreY(), 1.0f);
        return;
    }

    auto b = area.toFloat().reduced(4.0f, 2.0f);
    if (isHighlighted)
    {
        g.setColour(JPTheme::accent().withAlpha(0.22f));
        g.fillRoundedRectangle(b, 5.0f);
    }

    g.setFont(JPTheme::titleFont(12.8f));
    g.setColour(isActive ? JPTheme::accent().brighter(0.70f) : JPTheme::dimText().withAlpha(0.88f));
    auto textArea = area.reduced(13, 0);
    if (isTicked)
    {
        g.setColour(JPTheme::cyan());
        g.fillEllipse((float)textArea.getX(), (float)textArea.getCentreY() - 2.5f, 5.0f, 5.0f);
        textArea.removeFromLeft(12);
        g.setColour(JPTheme::accent().brighter(0.70f));
    }
    g.drawText(text, textArea, juce::Justification::centredLeft, false);
}


inline void JPOverlayLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height)
{
    auto b = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height).reduced(0.5f);

    g.setColour(JPTheme::accentDark().darker(0.05f));
    g.fillRoundedRectangle(b, 8.0f);

    g.setColour(JPTheme::accent().brighter(0.45f).withAlpha(0.55f));
    g.drawRoundedRectangle(b.reduced(0.5f), 8.0f, 1.2f);

    g.setColour(JPTheme::accent().withAlpha(0.20f));
    g.fillRoundedRectangle(b.reduced(2.0f).withTrimmedBottom(b.getHeight() * 0.55f), 7.0f);

    juce::AttributedString attributed;
    attributed.setJustification(juce::Justification::centredLeft);
    attributed.append(text, JPTheme::labelFont(11.6f), JPTheme::accent().brighter(0.70f));

    juce::TextLayout layout;
    layout.createLayoutWithBalancedLineLengths(attributed, (float)width - 16.0f);
    layout.draw(g, juce::Rectangle<float>(8.0f, 6.0f, (float)width - 16.0f, (float)height - 12.0f));
}


inline juce::Rectangle<int> JPOverlayLookAndFeel::getTooltipBounds(const juce::String& tipText,
    juce::Point<int> screenPos,
    juce::Rectangle<int> parentArea)
{
    const int textLength = tipText.length();
    const int preferredWidth = juce::jlimit(150, 340, 42 + textLength * 5);
    const int charsPerLine = juce::jmax(18, (preferredWidth - 24) / 6);
    const int lineCount = juce::jlimit(1, 5, 1 + textLength / charsPerLine);
    const int preferredHeight = juce::jlimit(30, 96, 20 + lineCount * 15);

    juce::Rectangle<int> bounds(screenPos.x + 18, screenPos.y + 18, preferredWidth, preferredHeight);

    if (bounds.getRight() > parentArea.getRight() - 8)
        bounds.setX(parentArea.getRight() - bounds.getWidth() - 8);

    if (bounds.getBottom() > parentArea.getBottom() - 8)
        bounds.setY(screenPos.y - bounds.getHeight() - 18);

    bounds.setX(juce::jmax(parentArea.getX() + 8, bounds.getX()));
    bounds.setY(juce::jmax(parentArea.getY() + 8, bounds.getY()));

    return bounds;
}

inline void JPOverlayLookAndFeel::drawButtonBackground(juce::Graphics& g,
    juce::Button& button,
    const juce::Colour&,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    auto b = button.getLocalBounds().toFloat().reduced(0.5f);
    const float radius = b.getHeight() * 0.5f;

    const bool accentButton = button.getButtonText().equalsIgnoreCase("Save")
        || button.getButtonText().equalsIgnoreCase("Delete")
        || button.getButtonText().equalsIgnoreCase("OK");

    g.setColour(accentButton
        ? JPTheme::accent().brighter(0.35f).withAlpha(shouldDrawButtonAsDown ? 0.34f : 0.25f)
        : JPTheme::accentDeep().withAlpha(shouldDrawButtonAsHighlighted ? 0.42f : 0.30f));
    g.fillRoundedRectangle(b, radius);

    g.setColour(JPTheme::accent().brighter(0.55f).withAlpha(shouldDrawButtonAsHighlighted || accentButton ? 0.72f : 0.44f));
    g.drawRoundedRectangle(b.reduced(0.5f), radius, 1.35f);
}

inline void JPOverlayLookAndFeel::drawButtonText(juce::Graphics& g,
    juce::TextButton& button,
    bool,
    bool)
{
    g.setFont(JPTheme::titleFont(13.0f));
    g.setColour(JPTheme::accent().brighter(0.70f));
    g.drawText(button.getButtonText(), button.getLocalBounds().reduced(4, 1), juce::Justification::centred, false);
}

inline void HardwareToggleButtonLookAndFeel::drawButtonBackground(juce::Graphics& g,
    juce::Button& button,
    const juce::Colour&,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    auto outer = button.getLocalBounds().toFloat().reduced(0.25f);
    const bool on = button.getToggleState();

    const juce::Colour lamp = on ? JPTheme::accent().brighter(0.55f) : JPTheme::accentDeep();
    const float radius = outer.getHeight() * 0.5f;

    g.setColour(on
        ? JPTheme::accent().brighter(0.35f).withAlpha(shouldDrawButtonAsDown ? 0.36f : 0.25f)
        : JPTheme::accentDeep().withAlpha(shouldDrawButtonAsHighlighted ? 0.42f : 0.30f));
    g.fillRoundedRectangle(outer, radius);

    if (shouldDrawButtonAsHighlighted)
    {
        g.setColour(JPTheme::accent().brighter(0.55f).withAlpha(0.10f));
        g.fillRoundedRectangle(outer.reduced(1.0f), radius);
    }

    g.setColour(lamp.withAlpha(on ? 0.72f : (shouldDrawButtonAsHighlighted ? 0.58f : 0.44f)));
    g.drawRoundedRectangle(outer.reduced(0.5f), radius, 1.4f);

}

inline void HardwareToggleButtonLookAndFeel::drawButtonText(juce::Graphics& g,
    juce::TextButton& button,
    bool,
    bool)
{
    const bool on = button.getToggleState();

    g.setFont(JPTheme::titleFont(12.5f));
    g.setColour(on ? JPTheme::accent().brighter(0.70f) : JPTheme::accent().brighter(0.38f));
    g.drawText(on ? "ON" : "OFF",
        button.getLocalBounds().reduced(4, 1),
        juce::Justification::centred,
        false);
}


inline void HardwareMeter::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    const bool isOutputMeter = title.startsWithIgnoreCase("OUTPUT");

    auto formatCompactDb = [](float valueDb)
        {
            if (valueDb <= -59.95f)
                return juce::String("-60");

            return juce::String(valueDb, 1);
        };

    const float peakDbValue = isOutputMeter
        ? truePeakHoldDbValue
        : juce::Decibels::gainToDecibels(juce::jmax(leftLevel, rightLevel), -60.0f);

    if (b.getWidth() < 120.0f && b.getHeight() > 110.0f)
    {
        // Open meter: no boxed card, only the lanes and readouts on the shared blue band.
        auto header = b.removeFromTop(26.0f).reduced(6.0f, 0.0f);
        g.setFont(JPTheme::titleFont(16.8f));
        g.setColour(JPTheme::accent().brighter(0.65f).withAlpha(0.92f));
        g.drawText(isOutputMeter ? "OUT" : "IN", header.withTrimmedTop(2.0f), juce::Justification::centred, false);

        auto footer = b.removeFromBottom(56.0f).reduced(5.0f, 2.0f);
        auto rmsRow = footer.removeFromTop(25.0f);
        auto peakRow = footer.removeFromTop(25.0f);

        auto drawReadout = [&](juce::Rectangle<float> row, const juce::String& name, float valueDb, bool alert)
            {
                g.setFont(JPTheme::titleFont(11.8f));
                g.setColour(alert ? JPTheme::red() : JPTheme::accent().brighter(0.25f).withAlpha(0.76f));
                g.drawText(name, row.removeFromLeft(44.0f), juce::Justification::centredLeft, false);

                g.setFont(JPTheme::titleFont(11.9f));
                g.setColour(alert ? JPTheme::red() : JPTheme::accent().brighter(0.65f).withAlpha(0.92f));
                g.drawText(formatCompactDb(valueDb), row, juce::Justification::centredRight, false);
            };

        drawReadout(rmsRow, "RMS", rmsDbValue, isOutputMeter && ispActive);
        drawReadout(peakRow, "PEAK", peakDbValue, isOutputMeter && ispActive);

        auto lanes = b.reduced(13.0f, 7.0f);
        const float laneGap = 7.0f;
        const float laneW = (lanes.getWidth() - laneGap) * 0.5f;
        auto leftLane = lanes.removeFromLeft(laneW);
        lanes.removeFromLeft(laneGap);
        auto rightLane = lanes.removeFromLeft(laneW);

        auto drawLane = [&](float level, float peak, juce::Rectangle<float> lane, const juce::String& channel)
            {
                auto label = lane.removeFromBottom(18.0f);
                auto track = lane.reduced(0.0f, 1.0f);

                // Meter colours follow the selected theme, but keep the same
                // hierarchy used by the blue skin: dark track, bright themed fill,
                // lighter peak line/readout. The only fixed colour is the red top
                // warning region, so clipping/ISP danger stays immediately readable.
                const auto meterTrackColour = JPTheme::band().darker(0.24f).withAlpha(0.82f);
                const auto meterAccent = JPTheme::accent();
                const auto meterDeep = JPTheme::accentDeep();

                g.setColour(meterTrackColour);
                g.fillRect(track);

                const float fillH = track.getHeight() * juce::jlimit(0.0f, 1.0f, level);
                auto fill = track.withTrimmedTop(track.getHeight() - fillH);

                juce::ColourGradient fillGradient(JPTheme::red().withAlpha(0.90f),
                    fill.getX(), fill.getY(),
                    meterAccent.withAlpha(0.96f),
                    fill.getX(), fill.getBottom(),
                    false);
                fillGradient.addColour(0.30, meterAccent.interpolatedWith(juce::Colours::white, 0.46f));

                g.setGradientFill(fillGradient);
                g.fillRect(fill);

                const float peakY = juce::jmap(juce::jlimit(0.0f, 1.0f, peak),
                    0.0f,
                    1.0f,
                    track.getBottom(),
                    track.getY());

                g.setColour(meterAccent.brighter(0.45f).withAlpha(0.72f));
                g.drawLine(track.getX() - 1.5f, peakY, track.getRight() + 1.5f, peakY, 1.0f);

                g.setColour(meterDeep.brighter(0.50f).withAlpha(0.18f));
                g.drawLine(track.getX(), track.getY(), track.getRight(), track.getY(), 1.0f);
                g.drawLine(track.getX(), track.getBottom(), track.getRight(), track.getBottom(), 1.0f);

                g.setFont(JPTheme::titleFont(13.8f));
                g.setColour(meterAccent.brighter(0.42f).withAlpha(0.86f));
                g.drawText(channel, label.translated(0.0f, 3.0f), juce::Justification::centred, false);
            };

        drawLane(leftLevel, leftPeakHold, leftLane, "L");
        drawLane(rightLevel, rightPeakHold, rightLane, "R");
        return;
    }

    g.setColour(JPTheme::panel().withAlpha(0.28f));
    g.fillRoundedRectangle(b, 7.0f);

    g.setColour(JPTheme::line().withAlpha(0.28f));
    g.drawRoundedRectangle(b.reduced(0.5f), 7.0f, 1.0f);

    auto header = b.removeFromTop(17.0f).reduced(8.0f, 0.0f);
    g.setFont(JPTheme::titleFont(9.8f));
    g.setColour(JPTheme::text().withAlpha(0.86f));
    g.drawText(isOutputMeter ? "OUT" : "IN", header.removeFromLeft(34.0f), juce::Justification::centredLeft, false);

    auto peakTextArea = header.removeFromRight(96.0f);
    g.setColour(isOutputMeter && ispActive ? JPTheme::red() : JPTheme::dimText().withAlpha(0.92f));
    g.setFont(JPTheme::labelFont(8.4f));
    g.drawText(isOutputMeter ? "TP" : "PK", peakTextArea.removeFromLeft(20.0f), juce::Justification::centredLeft, false);

    g.setColour(isOutputMeter && ispActive ? JPTheme::red() : JPTheme::text().withAlpha(0.88f));
    g.setFont(JPTheme::titleFont(9.3f));
    g.drawText(formatCompactDb(peakDbValue), peakTextArea, juce::Justification::centredRight, false);

    auto rmsTextArea = header.removeFromRight(92.0f);
    g.setColour(isOutputMeter && ispActive ? JPTheme::red() : JPTheme::dimText().withAlpha(0.92f));
    g.setFont(JPTheme::labelFont(8.4f));
    g.drawText("RMS", rmsTextArea.removeFromLeft(24.0f), juce::Justification::centredLeft, false);

    g.setColour(isOutputMeter && ispActive ? JPTheme::red() : JPTheme::text().withAlpha(0.92f));
    g.setFont(JPTheme::titleFont(9.3f));
    g.drawText(formatCompactDb(rmsDbValue), rmsTextArea, juce::Justification::centredRight, false);

    auto lanes = b.reduced(8.0f, 4.0f);
    auto leftLane = lanes.removeFromTop(9.0f);
    lanes.removeFromTop(5.0f);
    auto rightLane = lanes.removeFromTop(9.0f);

    auto drawLane = [&](float level, float peak, juce::Rectangle<float> lane, const juce::String& name)
        {
            auto labelArea = lane.removeFromLeft(13.0f);
            g.setFont(JPTheme::labelFont(8.0f));
            g.setColour(JPTheme::dimText().withAlpha(0.72f));
            g.drawText(name, labelArea, juce::Justification::centredLeft, false);

            auto track = lane.reduced(0.0f, 1.0f);
            g.setColour(JPTheme::band().darker(0.15f).withAlpha(0.86f));
            g.fillRect(track);

            const float fillW = track.getWidth() * juce::jlimit(0.0f, 1.0f, level);
            auto fill = track.withWidth(fillW);

            juce::ColourGradient fillGradient(JPTheme::cyan().withAlpha(0.95f),
                track.getX(), track.getY(),
                JPTheme::red().withAlpha(0.92f),
                track.getRight(), track.getY(),
                false);
            fillGradient.addColour(0.72, JPTheme::cyan().interpolatedWith(JPTheme::accent().brighter(0.80f), 0.55f));

            g.setGradientFill(fillGradient);
            g.fillRect(fill);

            const float peakX = juce::jmap(juce::jlimit(0.0f, 1.0f, peak),
                0.0f,
                1.0f,
                track.getX(),
                track.getRight());

            g.setColour(JPTheme::text().withAlpha(0.64f));
            g.drawLine(peakX, track.getY() - 1.0f, peakX, track.getBottom() + 1.0f, 1.0f);

            g.setColour(JPTheme::line().withAlpha(0.30f));
            g.drawRect(track.reduced(0.5f), 1.0f);
        };

    drawLane(leftLevel, leftPeakHold, leftLane, "L");
    drawLane(rightLevel, rightPeakHold, rightLane, "R");
}
