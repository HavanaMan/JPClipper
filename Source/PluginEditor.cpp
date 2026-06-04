#include "PluginEditor.h"

namespace
{
    constexpr int factoryPresetIdBase = 1000;
    constexpr int userPresetIdBase = 2000;
    constexpr int defaultEditorWidth = 1020;
    constexpr int defaultEditorHeight = 600;
    constexpr int minEditorWidth = 765;
    constexpr int minEditorHeight = 450;
    constexpr int maxEditorWidth = 1530;
    constexpr int maxEditorHeight = 900;
    constexpr const char* versionManifestUrl = "https://www.juicypads.com/plugins/versions.json";
    constexpr const char* pluginVersionKey = "JPClipper";

    juce::String getLocalPluginVersionString()
    {
       #if defined(JucePlugin_VersionString)
        return JucePlugin_VersionString;
       #else
        return ProjectInfo::versionString;
       #endif
    }

    std::array<int, 4> parseVersionParts(const juce::String& version)
    {
        std::array<int, 4> parts{};
        juce::StringArray tokens;
        tokens.addTokens(version.retainCharacters("0123456789."), ".", "");

        for (int i = 0; i < juce::jmin(4, tokens.size()); ++i)
            parts[(size_t)i] = tokens[i].getIntValue();

        return parts;
    }

    bool isRemoteVersionNewer(const juce::String& remoteVersion, const juce::String& localVersion)
    {
        const auto remote = parseVersionParts(remoteVersion);
        const auto local = parseVersionParts(localVersion);

        for (size_t i = 0; i < remote.size(); ++i)
        {
            if (remote[i] > local[i])
                return true;

            if (remote[i] < local[i])
                return false;
        }

        return false;
    }

    juce::String getStringProperty(const juce::var& object, const char* propertyName)
    {
        return object.getProperty(juce::Identifier(propertyName), {}).toString().trim();
    }

    juce::String getLatestVersionFromManifest(const juce::var& manifest)
    {
        const auto plugins = manifest.getProperty("plugins", {});
        auto pluginInfo = plugins.getProperty(pluginVersionKey, {});

        if (!pluginInfo.isObject())
            pluginInfo = manifest.getProperty(pluginVersionKey, {});

        if (!pluginInfo.isObject())
            pluginInfo = manifest;

        auto version = getStringProperty(pluginInfo, "latestVersion");

        if (version.isEmpty())
            version = getStringProperty(pluginInfo, "latest");

        if (version.isEmpty())
            version = getStringProperty(pluginInfo, "version");

        return version;
    }

    juce::String getUpdateUrlFromManifest(const juce::var& manifest)
    {
        const auto plugins = manifest.getProperty("plugins", {});
        auto pluginInfo = plugins.getProperty(pluginVersionKey, {});

        if (!pluginInfo.isObject())
            pluginInfo = manifest.getProperty(pluginVersionKey, {});

        if (!pluginInfo.isObject())
            pluginInfo = manifest;

        auto updateUrl = getStringProperty(pluginInfo, "downloadUrl");

        if (updateUrl.isEmpty())
            updateUrl = getStringProperty(pluginInfo, "url");

        return updateUrl;
    }

    float getTextWidth(const juce::Font& font, const juce::String& text)
    {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, text, 0.0f, 0.0f);
        return glyphs.getBoundingBox(0, glyphs.getNumGlyphs(), true).getWidth();
    }
}
void WaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // The parent paints the full-width blue band. Keep this component visually open.
    auto content = bounds.reduced(8.0f, 7.0f);

    auto metricStrip = content.removeFromBottom(30.0f);
    content.removeFromBottom(5.0f);

    // Clip history sits at the top of the waveform section and draws downward.
    // Keeping it above the main waveform avoids the old "bottom strip" feeling.
    auto clipHistoryArea = content.removeFromTop(26.0f);
    content.removeFromTop(6.0f);
    auto waveArea = content;

    const auto removedColour = JPTheme::cyan().interpolatedWith(JPTheme::green(), 0.35f);

    const float displayedReductionDb = juce::jlimit(0.0f, 60.0f, clipReductionDb);

    // ---------- WAVEFORM AREA ----------

    g.setColour(JPTheme::line().withAlpha(0.115f));

    for (int i = 1; i < 8; ++i)
    {
        const float x = juce::jmap((float)i,
            0.0f,
            8.0f,
            waveArea.getX(),
            waveArea.getRight());

        g.drawVerticalLine((int)x, waveArea.getY(), waveArea.getBottom());
    }

    for (int i = 1; i < 4; ++i)
    {
        const float y = juce::jmap((float)i,
            0.0f,
            4.0f,
            waveArea.getY(),
            waveArea.getBottom());

        g.drawHorizontalLine((int)y, waveArea.getX(), waveArea.getRight());
    }

    const int clampedScopeMode = juce::jlimit(0, 3, scopeMode);
    const bool showDeltaAsMainWaveform = clampedScopeMode == 2;
    const bool showPeaksOnly = clampedScopeMode == 3;
    const float displayRange = (showDeltaAsMainWaveform || showPeaksOnly) ? 0.5f : 1.0f;
    const float zeroY = waveArea.getCentreY();

    g.setColour(JPTheme::line().withAlpha(0.46f));
    g.drawHorizontalLine((int)zeroY, waveArea.getX(), waveArea.getRight());

    auto formatDb = [](float valueDb)
        {
            if (valueDb <= -59.95f)
                return juce::String("-60.0");

            return juce::String(valueDb, 1);
        };

    auto drawMetric = [&](const juce::String& name, const juce::String& value, juce::Rectangle<float> tag)
        {
            auto labelPart = tag.reduced(2.0f, 0.0f);
            const bool isReduction = name == "GR" && clipReductionDb > 0.05f;
            const float labelWidth = name == "PEAK" ? 40.0f : (name == "LUFS" ? 38.0f : (name == "CEIL" ? 36.0f : 32.0f));

            g.setFont(JPTheme::titleFont(14.6f));
            g.setColour(isReduction ? JPTheme::green().withAlpha(0.98f) : JPTheme::accent().brighter(0.55f).withAlpha(0.98f));
            g.drawText(name, labelPart.removeFromLeft(labelWidth), juce::Justification::centredLeft, false);

            g.setFont(JPTheme::titleFont(14.6f));
            g.setColour(isReduction ? JPTheme::green() : JPTheme::cyan());
            g.drawText(value, labelPart, juce::Justification::centredLeft, false);
        };

    if (!showDeltaAsMainWaveform && !showPeaksOnly)
    {
        const float limitedThreshold = juce::jlimit(0.0f, displayRange, threshold);

        const float thresholdY = juce::jmap(limitedThreshold,
            -displayRange, displayRange,
            waveArea.getBottom(), waveArea.getY());

        const float negativeThresholdY = juce::jmap(-limitedThreshold,
            -displayRange, displayRange,
            waveArea.getBottom(), waveArea.getY());

        auto drawCeilingLine = [&](float y)
            {
                juce::ColourGradient alert(JPTheme::accent().brighter(0.55f).withAlpha(0.36f),
                    waveArea.getX(), y,
                    JPTheme::green().withAlpha(0.07f),
                    waveArea.getRight(), y,
                    false);
                g.setGradientFill(alert);
                g.drawLine(waveArea.getX(), y, waveArea.getRight(), y, 1.25f);
            };

        drawCeilingLine(thresholdY);
        drawCeilingLine(negativeThresholdY);
    }

    if (clampedScopeMode == 1)
    {
        // IN/OUT:
        // dim blue = input/pre-clipper waveform
        // cyan = output/post-clipper waveform
        drawAudioWaveform(g,
            inputMinData,
            inputMaxData,
            waveArea,
            JPTheme::accent().withAlpha(0.34f),
            displayRange);

        drawAudioWaveform(g,
            clippedMinData,
            clippedMaxData,
            waveArea,
            JPTheme::accent(),
            displayRange);

        drawClipCrests(g,
            inputMinData,
            inputMaxData,
            clippedMinData,
            clippedMaxData,
            waveArea,
            removedColour,
            displayRange);
    }
    else if (showDeltaAsMainWaveform)
    {
        // DELTA:
        // grey = what remains after clipping
        // red = what the clipper removed
        drawAudioWaveform(g,
            clippedMinData,
            clippedMaxData,
            waveArea,
            JPTheme::dimText().withAlpha(0.50f),
            1.0f);

        drawAudioWaveform(g,
            removedMinData,
            removedMaxData,
            waveArea,
            removedColour,
            displayRange);
    }
    else if (showPeaksOnly)
    {
        // PEAKS:
        // only the material removed by the clipper.
        drawAudioWaveform(g,
            removedMinData,
            removedMaxData,
            waveArea,
            removedColour,
            displayRange);
    }
    else
    {
        // OUT:
        // only the output/post-clipper waveform.
        drawAudioWaveform(g,
            clippedMinData,
            clippedMaxData,
            waveArea,
            JPTheme::accent(),
            displayRange);
    }

    drawClipAmountHistory(g,
        removedMinData,
        removedMaxData,
        clipHistoryArea,
        removedColour,
        displayedReductionDb);

    g.setColour(JPTheme::line().withAlpha(0.18f));
    g.drawLine(metricStrip.getX(), metricStrip.getY(), metricStrip.getRight(), metricStrip.getY(), 1.0f);

    auto metrics = metricStrip.reduced(0.0f, 2.0f);
    drawMetric("RMS", formatDb(outputRmsDb), metrics.removeFromLeft(76.0f));
    metrics.removeFromLeft(5.0f);
    drawMetric("LUFS", formatDb(outputShortTermLufs), metrics.removeFromLeft(86.0f));
    metrics.removeFromLeft(5.0f);
    drawMetric("TP", formatDb(outputTruePeakDb), metrics.removeFromLeft(88.0f));
    metrics.removeFromLeft(6.0f);
    drawMetric("GR", juce::String::formatted("-%.1f", displayedReductionDb), metrics.removeFromLeft(72.0f));
    metrics.removeFromLeft(6.0f);
}

void WaveformDisplay::drawReductionWaveform(juce::Graphics& g,
    const std::array<float, JPClipperAudioProcessor::waveformSize>& minData,
    const std::array<float, JPClipperAudioProcessor::waveformSize>& maxData,
    juce::Rectangle<float> area,
    juce::Colour colour)
{
    juce::Path fillPath;
    juce::Path edgePath;

    const float topY = area.getY();
    const float scaleY = area.getHeight() * 1.35f;
    std::array<float, JPClipperAudioProcessor::waveformSize> reduction {};

    for (int i = 0; i < JPClipperAudioProcessor::waveformSize; ++i)
    {
        reduction[(size_t)i] = juce::jlimit(0.0f, 1.0f,
            juce::jmax(std::abs(minData[(size_t)i]),
                std::abs(maxData[(size_t)i])));
    }

    for (int i = 1; i < JPClipperAudioProcessor::waveformSize - 1; ++i)
    {
        reduction[(size_t)i] = reduction[(size_t)i - 1] * 0.22f
            + reduction[(size_t)i] * 0.56f
            + reduction[(size_t)i + 1] * 0.22f;
    }

    fillPath.startNewSubPath(area.getX(), topY);

    for (int i = 0; i < JPClipperAudioProcessor::waveformSize; ++i)
    {
        const float x = juce::jmap<float>((float)i,
            0.0f,
            (float)JPClipperAudioProcessor::waveformSize - 1.0f,
            area.getX(),
            area.getRight());

        const float y = juce::jlimit(area.getY(), area.getBottom(), topY + reduction[(size_t)i] * scaleY);

        if (i == 0)
        {
            fillPath.lineTo(x, y);
            edgePath.startNewSubPath(x, y);
        }
        else
        {
            const float previousX = juce::jmap<float>((float)i - 1.0f,
                0.0f,
                (float)JPClipperAudioProcessor::waveformSize - 1.0f,
                area.getX(),
                area.getRight());
            const float previousY = juce::jlimit(area.getY(), area.getBottom(), topY + reduction[(size_t)i - 1] * scaleY);
            const float midX = (previousX + x) * 0.5f;
            const float midY = (previousY + y) * 0.5f;
            fillPath.quadraticTo(previousX, previousY, midX, midY);
            edgePath.quadraticTo(previousX, previousY, midX, midY);
        }
    }

    const float lastX = area.getRight();
    const float lastY = juce::jlimit(area.getY(), area.getBottom(),
        topY + reduction[(size_t)JPClipperAudioProcessor::waveformSize - 1] * scaleY);
    fillPath.lineTo(lastX, lastY);
    edgePath.lineTo(lastX, lastY);

    fillPath.lineTo(area.getRight(), topY);
    fillPath.closeSubPath();

    g.setColour(colour.withAlpha(0.34f));
    g.fillPath(fillPath);

    g.setColour(colour.withAlpha(0.95f));
    g.strokePath(edgePath, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}


void WaveformDisplay::drawClipAmountHistory(juce::Graphics& g,
    const std::array<float, JPClipperAudioProcessor::waveformSize>& minData,
    const std::array<float, JPClipperAudioProcessor::waveformSize>& maxData,
    juce::Rectangle<float> area,
    juce::Colour colour,
    float currentReductionDb)
{
    juce::ignoreUnused(currentReductionDb);

    if (area.getWidth() <= 2.0f || area.getHeight() <= 2.0f)
        return;

    juce::Path fillPath;
    juce::Path edgePath;
    std::array<float, JPClipperAudioProcessor::waveformSize> history{};

    float peak = 0.0f;

    for (int i = 0; i < JPClipperAudioProcessor::waveformSize; ++i)
    {
        const float amount = juce::jlimit(0.0f, 1.0f,
            juce::jmax(std::abs(minData[(size_t)i]),
                std::abs(maxData[(size_t)i])));

        // Non-linear scaling keeps small clipping readable without making
        // heavy clipping dominate the whole waveform band.
        history[(size_t)i] = std::sqrt(amount);
        peak = juce::jmax(peak, history[(size_t)i]);
    }

    for (int i = 1; i < JPClipperAudioProcessor::waveformSize - 1; ++i)
    {
        history[(size_t)i] = history[(size_t)i - 1] * 0.20f
            + history[(size_t)i] * 0.60f
            + history[(size_t)i + 1] * 0.20f;
    }

    const float topY = area.getY();
    const float maxHeight = area.getHeight() * 0.72f;

    fillPath.startNewSubPath(area.getX(), topY);

    bool edgeStarted = false;

    for (int i = 0; i < JPClipperAudioProcessor::waveformSize; ++i)
    {
        const float x = juce::jmap<float>((float)i,
            0.0f,
            (float)JPClipperAudioProcessor::waveformSize - 1.0f,
            area.getX(),
            area.getRight());

        const float y = juce::jlimit(area.getY(), area.getBottom(),
            topY + history[(size_t)i] * maxHeight);

        if (i == 0)
        {
            fillPath.lineTo(x, y);
            edgePath.startNewSubPath(x, y);
            edgeStarted = true;
        }
        else
        {
            const float previousX = juce::jmap<float>((float)i - 1.0f,
                0.0f,
                (float)JPClipperAudioProcessor::waveformSize - 1.0f,
                area.getX(),
                area.getRight());

            const float previousY = juce::jlimit(area.getY(), area.getBottom(),
                topY + history[(size_t)i - 1] * maxHeight);

            const float midX = (previousX + x) * 0.5f;
            const float midY = (previousY + y) * 0.5f;

            fillPath.quadraticTo(previousX, previousY, midX, midY);
            edgePath.quadraticTo(previousX, previousY, midX, midY);
        }
    }

    fillPath.lineTo(area.getRight(), topY);
    fillPath.closeSubPath();

    juce::ColourGradient densityFill(colour.withAlpha(0.34f),
        area.getX(), area.getY(),
        colour.withAlpha(0.04f),
        area.getX(), area.getBottom(),
        false);

    g.setGradientFill(densityFill);
    g.fillPath(fillPath);

    if (edgeStarted)
    {
        const float strokeAlpha = peak > 0.015f ? 0.92f : 0.35f;
        g.setColour(colour.withAlpha(strokeAlpha));
        g.strokePath(edgePath, juce::PathStrokeType(1.55f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour(JPTheme::line().withAlpha(0.20f));
    g.drawLine(area.getX(), topY, area.getRight(), topY, 1.0f);
}

void WaveformDisplay::drawClipCrests(juce::Graphics& g,
    const std::array<float, JPClipperAudioProcessor::waveformSize>& inputMin,
    const std::array<float, JPClipperAudioProcessor::waveformSize>& inputMax,
    const std::array<float, JPClipperAudioProcessor::waveformSize>& clippedMin,
    const std::array<float, JPClipperAudioProcessor::waveformSize>& clippedMax,
    juce::Rectangle<float> area,
    juce::Colour colour,
    float displayRange)
{
    displayRange = juce::jmax(0.0001f, displayRange);

    auto sampleToY = [area, displayRange](float sample)
        {
            const float limited = juce::jlimit(-displayRange, displayRange, sample);

            return juce::jmap(limited,
                -displayRange, displayRange,
                area.getBottom(), area.getY());
        };

    juce::Path positiveCrests;
    juce::Path negativeCrests;
    bool hasPositive = false;
    bool hasNegative = false;

    for (int i = 0; i < JPClipperAudioProcessor::waveformSize; ++i)
    {
        const float x = juce::jmap<float>((float)i,
            0.0f,
            (float)JPClipperAudioProcessor::waveformSize - 1.0f,
            area.getX(),
            area.getRight());

        const float positiveRemoved = inputMax[(size_t)i] - clippedMax[(size_t)i];

        if (positiveRemoved > 0.0012f)
        {
            const float fromY = sampleToY(clippedMax[(size_t)i]);
            const float toY = sampleToY(inputMax[(size_t)i]);

            positiveCrests.startNewSubPath(x, fromY);
            positiveCrests.lineTo(x, toY);
            hasPositive = true;
        }

        const float negativeRemoved = clippedMin[(size_t)i] - inputMin[(size_t)i];

        if (negativeRemoved > 0.0012f)
        {
            const float fromY = sampleToY(clippedMin[(size_t)i]);
            const float toY = sampleToY(inputMin[(size_t)i]);

            negativeCrests.startNewSubPath(x, fromY);
            negativeCrests.lineTo(x, toY);
            hasNegative = true;
        }
    }

    g.setColour(colour.withAlpha(0.72f));

    if (hasPositive)
        g.strokePath(positiveCrests, juce::PathStrokeType(1.15f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (hasNegative)
        g.strokePath(negativeCrests, juce::PathStrokeType(1.15f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void WaveformDisplay::drawAudioWaveform(
    juce::Graphics& g,
    const std::array<float, JPClipperAudioProcessor::waveformSize>& minData,
    const std::array<float, JPClipperAudioProcessor::waveformSize>& maxData,
    juce::Rectangle<float> area,
    juce::Colour colour,
    float displayRange)
{
    constexpr int n = JPClipperAudioProcessor::waveformSize;

    displayRange = juce::jmax(0.0001f, displayRange);

    auto sampleToY = [area, displayRange](float sample)
        {
            const float limited = juce::jlimit(-displayRange, displayRange, sample);

            return juce::jmap(limited,
                -displayRange, displayRange,
                area.getBottom(), area.getY());
        };

    std::array<juce::Point<float>, JPClipperAudioProcessor::waveformSize> upperPoints {};
    std::array<juce::Point<float>, JPClipperAudioProcessor::waveformSize> lowerPoints {};
    std::array<float, JPClipperAudioProcessor::waveformSize> upperSamples {};
    std::array<float, JPClipperAudioProcessor::waveformSize> lowerSamples {};

    for (int i = 0; i < n; ++i)
    {
        const float lo = juce::jmin(minData[(size_t)i], maxData[(size_t)i]);
        const float hi = juce::jmax(minData[(size_t)i], maxData[(size_t)i]);

        upperSamples[(size_t)i] = hi;
        lowerSamples[(size_t)i] = lo;
    }

    for (int pass = 0; pass < 2; ++pass)
    {
        auto smoothedUpper = upperSamples;
        auto smoothedLower = lowerSamples;

        for (int i = 1; i < n - 1; ++i)
        {
            smoothedUpper[(size_t)i] = upperSamples[(size_t)i - 1] * 0.20f
                + upperSamples[(size_t)i] * 0.60f
                + upperSamples[(size_t)i + 1] * 0.20f;

            smoothedLower[(size_t)i] = lowerSamples[(size_t)i - 1] * 0.20f
                + lowerSamples[(size_t)i] * 0.60f
                + lowerSamples[(size_t)i + 1] * 0.20f;
        }

        upperSamples = smoothedUpper;
        lowerSamples = smoothedLower;
    }

    for (int i = 0; i < n; ++i)
    {
        const float x = juce::jmap<float>((float)i,
            0.0f,
            (float)n - 1.0f,
            area.getX(),
            area.getRight());

        upperPoints[(size_t)i] = { x, sampleToY(upperSamples[(size_t)i]) };
        lowerPoints[(size_t)i] = { x, sampleToY(lowerSamples[(size_t)i]) };
    }

    juce::Path body;
    body.startNewSubPath(upperPoints[0].x, upperPoints[0].y);

    for (int i = 1; i < n - 1; ++i)
    {
        const auto current = upperPoints[(size_t)i];
        const auto next = upperPoints[(size_t)i + 1];
        const auto mid = juce::Point<float>((current.x + next.x) * 0.5f,
                                            (current.y + next.y) * 0.5f);

        body.quadraticTo(current.x, current.y, mid.x, mid.y);
    }

    body.lineTo(upperPoints[(size_t)n - 1].x, upperPoints[(size_t)n - 1].y);
    body.lineTo(lowerPoints[(size_t)n - 1].x, lowerPoints[(size_t)n - 1].y);

    for (int i = n - 2; i > 0; --i)
    {
        const auto current = lowerPoints[(size_t)i];
        const auto prev = lowerPoints[(size_t)i - 1];
        const auto mid = juce::Point<float>((current.x + prev.x) * 0.5f,
                                            (current.y + prev.y) * 0.5f);

        body.quadraticTo(current.x, current.y, mid.x, mid.y);
    }

    body.lineTo(lowerPoints[0].x, lowerPoints[0].y);
    body.closeSubPath();

    g.setColour(colour.withAlpha(0.18f));
    g.fillPath(body);

    juce::Path upperEdge;
    upperEdge.startNewSubPath(upperPoints[0].x, upperPoints[0].y);

    for (int i = 1; i < n - 1; ++i)
    {
        const auto current = upperPoints[(size_t)i];
        const auto next = upperPoints[(size_t)i + 1];
        const auto mid = juce::Point<float>((current.x + next.x) * 0.5f,
                                            (current.y + next.y) * 0.5f);

        upperEdge.quadraticTo(current.x, current.y, mid.x, mid.y);
    }

    upperEdge.lineTo(upperPoints[(size_t)n - 1].x, upperPoints[(size_t)n - 1].y);

    juce::Path lowerEdge;
    lowerEdge.startNewSubPath(lowerPoints[0].x, lowerPoints[0].y);

    for (int i = 1; i < n - 1; ++i)
    {
        const auto current = lowerPoints[(size_t)i];
        const auto next = lowerPoints[(size_t)i + 1];
        const auto mid = juce::Point<float>((current.x + next.x) * 0.5f,
                                            (current.y + next.y) * 0.5f);

        lowerEdge.quadraticTo(current.x, current.y, mid.x, mid.y);
    }

    lowerEdge.lineTo(lowerPoints[(size_t)n - 1].x, lowerPoints[(size_t)n - 1].y);

    g.setColour(colour.withAlpha(0.78f));
    g.strokePath(upperEdge, juce::PathStrokeType(1.35f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.strokePath(lowerEdge, juce::PathStrokeType(1.35f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

JPClipperAudioProcessorEditor::JPClipperAudioProcessorEditor(JPClipperAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setResizable(true, true);
    setResizeLimits(minEditorWidth, minEditorHeight, maxEditorWidth, maxEditorHeight);

    if (auto* editorConstrainer = getConstrainer())
        editorConstrainer->setFixedAspectRatio((double)defaultEditorWidth / (double)defaultEditorHeight);

    constexpr float defaultStartupScale = 1.10f;

    const int savedEditorWidth = audioProcessor.getEditorWidth();
    const int savedEditorHeight = audioProcessor.getEditorHeight();

    const bool usingLegacyDefaultSize = savedEditorWidth == defaultEditorWidth
        && savedEditorHeight == defaultEditorHeight;

    const float savedWidthScale = (float)savedEditorWidth / (float)defaultEditorWidth;
    const float savedHeightScale = (float)savedEditorHeight / (float)defaultEditorHeight;
    const float requestedInitialScale = usingLegacyDefaultSize
        ? defaultStartupScale
        : juce::jmin(savedWidthScale, savedHeightScale);

    const float initialScale = juce::jlimit((float)minEditorWidth / (float)defaultEditorWidth,
        (float)maxEditorWidth / (float)defaultEditorWidth,
        requestedInitialScale);

    setSize((int)std::round((float)defaultEditorWidth * initialScale),
        (int)std::round((float)defaultEditorHeight * initialScale));

    setLookAndFeel(&jpOverlayLookAndFeel);
    tooltipWindow.setLookAndFeel(&jpOverlayLookAndFeel);

    inputMeter.setTitle("INPUT");
    outputMeter.setTitle("OUTPUT");

    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(waveformDisplay);

    presetBox.setJustificationType(juce::Justification::centred);
    presetBox.setLookAndFeel(&blueComboBoxLookAndFeel);
    presetBox.setTextWhenNothingSelected("Presets");
    presetBox.setColour(juce::ComboBox::backgroundColourId, JPTheme::accentDark().darker(0.10f).withAlpha(0.82f));
    presetBox.setColour(juce::ComboBox::outlineColourId, JPTheme::accent().brighter(0.45f).withAlpha(0.34f));
    presetBox.setColour(juce::ComboBox::textColourId, JPTheme::accent().brighter(0.65f));
    presetBox.setColour(juce::ComboBox::arrowColourId, JPTheme::accent().brighter(0.55f));
    presetBox.setColour(juce::PopupMenu::backgroundColourId, JPTheme::accentDeep());
    presetBox.setColour(juce::PopupMenu::textColourId, JPTheme::accent().brighter(0.65f));

    presetBox.onChange = [this] { loadSelectedPreset(); };
    presetBox.setTooltip("Factory presets are read-only. User presets can be saved and deleted with SAVE/DEL.");
    addAndMakeVisible(presetBox);


    oversamplingBox.setJustificationType(juce::Justification::centred);
    oversamplingBox.setLookAndFeel(&blueComboBoxLookAndFeel);
    oversamplingBox.setTextWhenNothingSelected("OFF");
    oversamplingBox.addItem("OFF", 1);
    oversamplingBox.addItem("2X", 2);
    oversamplingBox.addItem("4X", 3);
    oversamplingBox.addItem("8X", 4);
    oversamplingBox.addItem("16X", 5);
    oversamplingBox.addItem("32X", 6);
    oversamplingBox.setColour(juce::ComboBox::backgroundColourId, JPTheme::accentDeep().withAlpha(0.78f));
    oversamplingBox.setColour(juce::ComboBox::outlineColourId, JPTheme::accent().brighter(0.45f).withAlpha(0.48f));
    oversamplingBox.setColour(juce::ComboBox::textColourId, JPTheme::accent().brighter(0.65f));
    oversamplingBox.setColour(juce::ComboBox::arrowColourId, JPTheme::accent().brighter(0.55f));
    oversamplingBox.setColour(juce::PopupMenu::backgroundColourId, JPTheme::accentDeep());
    oversamplingBox.setColour(juce::PopupMenu::textColourId, JPTheme::accent().brighter(0.65f));
    oversamplingBox.onChange = [this]
        {
            const int selected = oversamplingBox.getSelectedId();
            if (selected > 0)
                setOversamplingChoice(selected - 1);
        };
    oversamplingBox.setTooltip("Oversampling amount: OFF, 2X, 4X, 8X, 16X or 32X.");
    addAndMakeVisible(oversamplingBox);


    processingModeBox.setJustificationType(juce::Justification::centred);
    processingModeBox.setLookAndFeel(&blueComboBoxLookAndFeel);
    processingModeBox.setTextWhenNothingSelected("STEREO");
    processingModeBox.addItem("STEREO", 1);
    processingModeBox.addItem("MID", 2);
    processingModeBox.addItem("SIDE", 3);
    processingModeBox.addItem("M+S", 4);
    processingModeBox.setColour(juce::ComboBox::backgroundColourId, JPTheme::accentDeep().withAlpha(0.78f));
    processingModeBox.setColour(juce::ComboBox::outlineColourId, JPTheme::accent().brighter(0.45f).withAlpha(0.48f));
    processingModeBox.setColour(juce::ComboBox::textColourId, JPTheme::accent().brighter(0.65f));
    processingModeBox.setColour(juce::ComboBox::arrowColourId, JPTheme::accent().brighter(0.55f));
    processingModeBox.setColour(juce::PopupMenu::backgroundColourId, JPTheme::accentDeep());
    processingModeBox.setColour(juce::PopupMenu::textColourId, JPTheme::accent().brighter(0.65f));
    processingModeBox.onChange = [this]
        {
            const int selected = processingModeBox.getSelectedId();
            if (selected > 0)
                setProcessingModeChoice(selected - 1);
        };
    processingModeBox.setTooltip("Processing domain: STEREO, MID only, SIDE only, or independent M+S clipping.");
    addAndMakeVisible(processingModeBox);

    scopeTypeBox.setJustificationType(juce::Justification::centred);
    scopeTypeBox.setLookAndFeel(&blueComboBoxLookAndFeel);
    scopeTypeBox.setTextWhenNothingSelected("IN/OUT");
    scopeTypeBox.addItem("OUT", 1);
    scopeTypeBox.addItem("IN/OUT", 2);
    scopeTypeBox.addItem("DELTA", 3);
    scopeTypeBox.addItem("PEAKS", 4);
    scopeTypeBox.setColour(juce::ComboBox::backgroundColourId, JPTheme::accentDeep().withAlpha(0.78f));
    scopeTypeBox.setColour(juce::ComboBox::outlineColourId, JPTheme::accent().brighter(0.45f).withAlpha(0.48f));
    scopeTypeBox.setColour(juce::ComboBox::textColourId, JPTheme::accent().brighter(0.65f));
    scopeTypeBox.setColour(juce::ComboBox::arrowColourId, JPTheme::accent().brighter(0.55f));
    scopeTypeBox.setColour(juce::PopupMenu::backgroundColourId, JPTheme::accentDeep());
    scopeTypeBox.setColour(juce::PopupMenu::textColourId, JPTheme::accent().brighter(0.65f));
    scopeTypeBox.onChange = [this]
        {
            const int selected = scopeTypeBox.getSelectedId();
            if (selected > 0)
                setScopeChoice(selected - 1);
        };
    scopeTypeBox.setTooltip("Waveform display type: OUT, IN/OUT, DELTA or PEAKS.");
    addAndMakeVisible(scopeTypeBox);

    scopeZoomBox.setJustificationType(juce::Justification::centred);
    scopeZoomBox.setLookAndFeel(&blueComboBoxLookAndFeel);
    scopeZoomBox.setTextWhenNothingSelected("3X");
    for (int i = 1; i <= 8; ++i)
        scopeZoomBox.addItem(juce::String(i) + "X", i);
    scopeZoomBox.setColour(juce::ComboBox::backgroundColourId, JPTheme::accentDeep().withAlpha(0.78f));
    scopeZoomBox.setColour(juce::ComboBox::outlineColourId, JPTheme::accent().brighter(0.45f).withAlpha(0.48f));
    scopeZoomBox.setColour(juce::ComboBox::textColourId, JPTheme::accent().brighter(0.65f));
    scopeZoomBox.setColour(juce::ComboBox::arrowColourId, JPTheme::accent().brighter(0.55f));
    scopeZoomBox.setColour(juce::PopupMenu::backgroundColourId, JPTheme::accentDeep());
    scopeZoomBox.setColour(juce::PopupMenu::textColourId, JPTheme::accent().brighter(0.65f));
    scopeZoomBox.onChange = [this]
        {
            const int selected = scopeZoomBox.getSelectedId();
            if (selected <= 0)
                return;

            if (auto* scopeZoomParam = audioProcessor.apvts.getParameter("scopeZoom"))
            {
                const float normalised = juce::jlimit(0.0f, 1.0f, (float)(selected - 1) / 7.0f);
                audioProcessor.getUndoManager().beginNewTransaction("Set Scope Time");
                scopeZoomParam->beginChangeGesture();
                scopeZoomParam->setValueNotifyingHost(normalised);
                scopeZoomParam->endChangeGesture();
            }
        };
    scopeZoomBox.setTooltip("Scope time window: higher values make the waveform move slower.");
    addAndMakeVisible(scopeZoomBox);

    footerLink.setButtonText("JUICYPADS.COM");
    footerLink.setURL(juce::URL("https://www.juicypads.com"));
    footerLink.setColour(juce::HyperlinkButton::textColourId, JPTheme::accent().brighter(0.65f));
    footerLink.setTooltip("Open the Juicy Pads website.");
    footerLink.setFont(JPTheme::titleFont(13.2f), false);
    addAndMakeVisible(footerLink);

    versionLabel.setText("V." + getLocalPluginVersionString(), juce::dontSendNotification);
    versionLabel.setJustificationType(juce::Justification::centredRight);
    versionLabel.setColour(juce::Label::textColourId, JPTheme::accent().brighter(0.25f).withAlpha(0.92f));
    versionLabel.setFont(JPTheme::titleFont(13.2f));
    versionLabel.setTooltip("Plugin version from the build metadata.");
    addAndMakeVisible(versionLabel);

    // Kept allocated for compatibility with the existing layout/look-and-feel code,
    // but the UI no longer shows an "UPDATE AVAILABLE" badge. When an update is
    // found the version label itself becomes red.
    setupSmallButton(updateAvailableButton);
    updateAvailableButton.setButtonText("");
    updateAvailableButton.setName("update");
    updateAvailableButton.setToggleState(false, juce::dontSendNotification);
    updateAvailableButton.setLookAndFeel(&hardwareButtonLookAndFeel);
    updateAvailableButton.setVisible(false);
    addAndMakeVisible(updateAvailableButton);

    themeBox.setJustificationType(juce::Justification::centred);
    themeBox.setLookAndFeel(&blueComboBoxLookAndFeel);
    themeBox.addItem("BLUE", JPTheme::blue + 1);
    themeBox.addItem("BLACK", JPTheme::black + 1);
    themeBox.addItem("GREEN", JPTheme::greenTheme + 1);
    themeBox.addItem("PINK", JPTheme::pink + 1);
    themeBox.addItem("YELLOW", JPTheme::yellow + 1);
    themeBox.addItem("PURPLE", JPTheme::purple + 1);
    themeBox.addItem("ORANGE", JPTheme::orange + 1);
    themeBox.setSelectedId(JPTheme::getVariant() + 1, juce::dontSendNotification);
    themeBox.setColour(juce::ComboBox::backgroundColourId, JPTheme::accentDeep().withAlpha(0.78f));
    themeBox.setColour(juce::ComboBox::outlineColourId, JPTheme::accent().brighter(0.45f).withAlpha(0.48f));
    themeBox.setColour(juce::ComboBox::textColourId, JPTheme::accent().brighter(0.65f));
    themeBox.setColour(juce::ComboBox::arrowColourId, JPTheme::accent().brighter(0.55f));
    themeBox.setColour(juce::PopupMenu::backgroundColourId, JPTheme::accentDeep());
    themeBox.setColour(juce::PopupMenu::textColourId, JPTheme::accent().brighter(0.65f));
    themeBox.setTooltip("Choose the JP template colour theme.");
    themeBox.onChange = [this]
        {
            const int selected = themeBox.getSelectedId();
            if (selected > 0)
                setThemeVariant(selected - 1);
        };
    addAndMakeVisible(themeBox);
    updateThemeButtons();

    setupSmallButton(savePresetButton);
    setupSmallButton(deletePresetButton);
    savePresetButton.setName("save");
    deletePresetButton.setName("delete");
    savePresetButton.setButtonText("");
    deletePresetButton.setButtonText("");
    savePresetButton.setLookAndFeel(&headerIconButtonLookAndFeel);
    deletePresetButton.setLookAndFeel(&headerIconButtonLookAndFeel);
    savePresetButton.setTooltip("Save the current settings as a user preset.");
    deletePresetButton.setTooltip("Delete the selected user preset. Factory presets cannot be deleted.");
    savePresetButton.onClick = [this] { showSavePresetDialog(); };
    deletePresetButton.onClick = [this] { deleteSelectedUserPreset(); };

    setupSmallButton(aButton);
    setupSmallButton(bButton);
    setupSmallButton(copyABButton);
    setupSmallButton(pasteSettingsButton);
    setupSmallButton(undoButton);
    setupSmallButton(redoButton);
    setupSmallButton(helpButton);

    aButton.setButtonText("A");
    bButton.setButtonText("B");
    copyABButton.setName("copy");
    pasteSettingsButton.setName("paste");
    undoButton.setName("undo");
    redoButton.setName("redo");
    helpButton.setName("help");
    copyABButton.setButtonText("");
    pasteSettingsButton.setButtonText("");
    undoButton.setButtonText("");
    redoButton.setButtonText("");
    helpButton.setButtonText("?");
    copyABButton.setLookAndFeel(&headerIconButtonLookAndFeel);
    pasteSettingsButton.setLookAndFeel(&headerIconButtonLookAndFeel);
    undoButton.setLookAndFeel(&headerIconButtonLookAndFeel);
    redoButton.setLookAndFeel(&headerIconButtonLookAndFeel);
    helpButton.setLookAndFeel(&headerIconButtonLookAndFeel);
    helpButton.setClickingTogglesState(true);
    tooltipWindow.setMillisecondsBeforeTipAppears(60 * 60 * 1000);
    aButton.setTooltip("Recall A settings for quick comparison.");
    bButton.setTooltip("Recall B settings for quick comparison.");
    copyABButton.setTooltip("Copy this instance's settings to the system clipboard.");
    pasteSettingsButton.setTooltip("Paste JP|CLIPPER settings from the system clipboard into this instance.");
    undoButton.setTooltip("Undo the last parameter or preset change.");
    redoButton.setTooltip("Redo the last undone change.");
    helpButton.setTooltip("Enable contextual tooltips.");

    aButton.onClick = [this]
        {
            audioProcessor.getUndoManager().beginNewTransaction("Recall A");
            audioProcessor.selectABSlot(0);
            updateABButtons();
        };

    bButton.onClick = [this]
        {
            audioProcessor.getUndoManager().beginNewTransaction("Recall B");
            audioProcessor.selectABSlot(1);
            updateABButtons();
        };

    copyABButton.onClick = [this]
        {
            juce::SystemClipboard::copyTextToClipboard(audioProcessor.createClipboardSettingsText());
        };

    pasteSettingsButton.onClick = [this]
        {
            if (!audioProcessor.loadClipboardSettingsText(juce::SystemClipboard::getTextFromClipboard()))
                return;

            refreshPresetBox();
            updateModeButtons();
            updateOversamplingButtons();
            updateProcessingModeBox();
            updatePhaseButtons();
            updateLiftButtons();
            updateScopeButtons();
            updateABButtons();
            updateUtilityButtons();
        };

    undoButton.onClick = [this]
        {
            audioProcessor.getUndoManager().undo();
            updateUtilityButtons();
        };

    redoButton.onClick = [this]
        {
            audioProcessor.getUndoManager().redo();
            updateUtilityButtons();
        };

    helpButton.onClick = [this]
        {
            tooltipsEnabled = helpButton.getToggleState();
            tooltipWindow.setMillisecondsBeforeTipAppears(tooltipsEnabled ? 700 : 60 * 60 * 1000);

            if (!tooltipsEnabled)
                tooltipWindow.hideTip();
        };

    setupSlider(inputSlider);
    setupSlider(thresholdSlider);
    setupSlider(kneeSlider);
    setupSlider(outputSlider);
    setupSlider(ceilingSlider);
    setupSlider(mixSlider);
    setupSlider(colorSlider);
    setupSlider(stereoLinkSlider);
    setupSlider(scopeZoomSlider);
    setupSlider(liftSlider);
    setupSlider(spacerSlider);
    spacerSlider.setName(" ");
    spacerSlider.setRange(0.0, 1.0, 0.01);
    spacerSlider.setValue(0.62, juce::dontSendNotification);
    spacerSlider.setInterceptsMouseClicks(false, false);
    spacerSlider.setAlpha(0.44f);
    inputSlider.setName("IN");
    thresholdSlider.setName("THRESHOLD");
    kneeSlider.setName("KNEE");
    outputSlider.setName("OUT");
    ceilingSlider.setName("CEILING");
    mixSlider.setName("MIX");
    colorSlider.setName("COLOR");
    liftSlider.setName("LIFT");
    stereoLinkSlider.setName("ST LINK");
    scopeZoomSlider.setName("SCOPE X");
    stereoLinkSlider.setRange(0.0, 100.0, 0.01);
    stereoLinkSlider.setDoubleClickReturnValue(true, 100.0);
    liftSlider.setRange(0.0, 3.0, 1.0);
    liftSlider.setDoubleClickReturnValue(true, 0.0);
    liftSlider.setNumDecimalPlacesToDisplay(0);
    liftSlider.setTooltip("Lift modes: Off, Body, Dense, Push.");
    liftSlider.onValueChange = [this]
        {
            setLiftChoice((int)std::round(liftSlider.getValue()));
        };

    setupToggleButton(autoGainButton);
    setupToggleButton(matchButton);
    setupToggleButton(dcBlockerButton);
    setupToggleButton(deltaButton);
    setupToggleButton(bypassButton);

    autoGainButton.setButtonText("AUTO");
    matchButton.setButtonText("MATCH");
    dcBlockerButton.setButtonText("DC");
    deltaButton.setButtonText("DELTA");
    bypassButton.setButtonText("");
    bypassButton.setName("bypass");
    bypassButton.setLookAndFeel(&headerIconButtonLookAndFeel);
    matchButton.setTooltip("Slowly matches processed RMS to input RMS without moving the Output knob.");
    matchAmountLabel.setTooltip("Current internal MATCH compensation in dB.");
    autoGainButton.setTooltip("Auto Gain applies internal compensation opposite to the Input trim without moving the Output knob.");
    dcBlockerButton.setTooltip("Remove DC offset before processing.");
    deltaButton.setTooltip("Listen to the material removed by the clipper.");
    bypassButton.setTooltip("Power: ON means JP|CLIPPER is processing; OFF means the processor is bypassed.");

    setupLabel(inputLabel, "IN");
    setupLabel(thresholdLabel, "THRESHOLD");
    setupLabel(kneeLabel, "KNEE");
    setupLabel(outputLabel, "OUT");
    setupLabel(ceilingLabel, "CEILING");
    setupLabel(mixLabel, "MIX");
    setupLabel(colorLabel, "COLOR");
    setupLabel(stereoLinkLabel, "ST LINK");
    setupLabel(scopeZoomLabel, "SCOPE X");
    setupLabel(inputValueLabel, "0.0 dB");
    setupLabel(thresholdValueLabel, "0.0 dB");
    setupLabel(kneeValueLabel, "0%");
    setupLabel(outputValueLabel, "0.0 dB");
    setupLabel(ceilingValueLabel, "0.0 dB");
    setupLabel(mixValueLabel, "100%");
    setupLabel(colorValueLabel, "0%");
    setupLabel(stereoLinkValueLabel, "100%");
    setupLabel(scopeZoomValueLabel, "3X");
    setupLabel(spacerLabel, "");
    setupLabel(spacerValueLabel, "");
    spacerLabel.setInterceptsMouseClicks(false, false);
    spacerValueLabel.setInterceptsMouseClicks(false, false);
    for (auto* valueLabel : { &inputValueLabel, &thresholdValueLabel, &kneeValueLabel,
        &outputValueLabel, &ceilingValueLabel, &mixValueLabel, &colorValueLabel, &stereoLinkValueLabel, &scopeZoomValueLabel })
    {
        valueLabel->setColour(juce::Label::textColourId, JPTheme::text().withAlpha(0.92f));
        valueLabel->setFont(JPTheme::titleFont(12.0f));
    }

    setupLabel(autoGainLabel, "AUTO GAIN");
    setupLabel(matchLabel, "LEVEL MATCH");
    setupLabel(matchAmountLabel, "");
    matchAmountLabel.setVisible(false);
    setupLabel(dcBlockerLabel, "DC OFFSET");
    setupLabel(deltaLabel, "MONITOR");
    setupLabel(bypassLabel, "BYPASS");
    setupLabel(modeLabel, "CLIP MODE");
    setupLabel(processingModeLabel, "MODE");
    setupLabel(liftLabel, "LIFT");
    setupLabel(liftValueLabel, "OFF");
    liftValueLabel.setColour(juce::Label::textColourId, JPTheme::accent());
    liftValueLabel.setFont(JPTheme::titleFont(12.0f));
    setupLabel(oversamplingLabel, "OVERSAMPLING");
    setupLabel(phaseLabel, "PHASE");
    setupLabel(scopeLabel, "SCOPE");

    waveformDisplay.setTooltip("Main display: cyan shows the selected waveform view; green/cyan highlights show clipping and reduction activity.");
    inputMeter.setTooltip("Stereo input meter with RMS and peak readout.");
    outputMeter.setTooltip("Stereo output meter with RMS, true peak and peak hold.");
    inputSlider.setTooltip("Input trim before clipping.");
    inputLabel.setTooltip("Input trim before clipping.");
    inputValueLabel.setTooltip("Current input trim in dB.");
    thresholdSlider.setTooltip("Clip threshold. Lower values shave more peaks.");
    thresholdLabel.setTooltip("Clip threshold. Lower values shave more peaks.");
    thresholdValueLabel.setTooltip("Current clip threshold in dB.");
    kneeSlider.setTooltip("Soft knee amount for the soft clip mode.");
    kneeLabel.setTooltip("Soft knee amount for the soft clip mode.");
    kneeValueLabel.setTooltip("Current soft knee amount.");
    outputSlider.setTooltip("Output gain after the clipping stage.");
    outputLabel.setTooltip("Output gain after the clipping stage.");
    outputValueLabel.setTooltip("Current output gain in dB.");
    ceilingSlider.setTooltip("Final output ceiling in dB.");
    ceilingLabel.setTooltip("Final output ceiling in dB.");
    ceilingValueLabel.setTooltip("Current ceiling value in dB.");
    mixSlider.setTooltip("Dry/wet blend between original and clipped signal.");
    mixLabel.setTooltip("Dry/wet blend between original and clipped signal.");
    mixValueLabel.setTooltip("Current dry/wet blend.");
    colorSlider.setTooltip("Adds subtle clipper colour after the clipping stage.");
    colorLabel.setTooltip("Adds subtle clipper colour after the clipping stage.");
    colorValueLabel.setTooltip("Current colour amount.");
    stereoLinkSlider.setTooltip("Stereo link: 0% clips left/right independently, 100% keeps clipping gain reduction linked.");
    stereoLinkLabel.setTooltip("Stereo link amount.");
    stereoLinkValueLabel.setTooltip("Current stereo link amount.");
    scopeZoomSlider.setTooltip("Scope time window: higher values make the waveform move slower.");
    scopeZoomLabel.setTooltip("Scope time window: higher values make the waveform move slower.");
    scopeZoomValueLabel.setTooltip("Current scope time window.");
    scopeZoomSlider.setVisible(false);
    scopeZoomLabel.setVisible(false);
    scopeZoomValueLabel.setVisible(false);
    liftLabel.setTooltip("Lift waveshaping mode.");
    liftValueLabel.setTooltip("Current lift mode.");
    autoGainLabel.setTooltip("Internal compensation opposite to the Input trim. It does not move the Output knob.");
    matchLabel.setTooltip("Slow RMS matching group.");
    dcBlockerLabel.setTooltip("DC offset removal.");
    deltaLabel.setTooltip("Monitoring utilities.");
    bypassLabel.setTooltip("Global processor bypass.");
    modeLabel.setTooltip("Clipper response mode.");
    processingModeLabel.setTooltip("Stereo / Mid / Side / M+S processing mode.");
    oversamplingLabel.setTooltip("Oversampling amount.");
    phaseLabel.setTooltip("Oversampling filter phase mode.");
    scopeLabel.setTooltip("Waveform display mode.");

    setupSmallButton(hardButton);
    setupSmallButton(softButton);
    setupSmallButton(linearPhaseButton);
    setupSmallButton(minPhaseButton);

    hardButton.setButtonText("HARD");
    softButton.setButtonText("SOFT");
    linearPhaseButton.setButtonText("LIN");
    minPhaseButton.setButtonText("MIN");
    hardButton.setTooltip("Hard sample-by-sample peak clipping.");
    softButton.setTooltip("Softer clipping curve around the threshold.");
    linearPhaseButton.setTooltip("Linear phase oversampling filter mode.");
    minPhaseButton.setTooltip("Minimum phase oversampling filter mode.");

    hardButton.onClick = [this] { setModeChoice(0); };
    softButton.onClick = [this] { setModeChoice(1); };
    linearPhaseButton.onClick = [this] { setPhaseChoice(0); };
    minPhaseButton.onClick = [this] { setPhaseChoice(1); };

    setupSmallButton(osOffButton);
    setupSmallButton(os2Button);
    setupSmallButton(os4Button);
    setupSmallButton(os8Button);
    setupSmallButton(os16Button);
    setupSmallButton(os32Button);

    osOffButton.setButtonText("OFF");
    os2Button.setButtonText("2X");
    os4Button.setButtonText("4X");
    os8Button.setButtonText("8X");
    os16Button.setButtonText("16X");
    os32Button.setButtonText("32X");
    osOffButton.setTooltip("Disable oversampling.");
    os2Button.setTooltip("Process the clipper at 2x oversampling.");
    os4Button.setTooltip("Process the clipper at 4x oversampling.");
    os8Button.setTooltip("Process the clipper at 8x oversampling.");
    os16Button.setTooltip("Process the clipper at 16x oversampling.");
    os32Button.setTooltip("Process the clipper at 32x oversampling.");
    osOffButton.setVisible(false);
    os2Button.setVisible(false);
    os4Button.setVisible(false);
    os8Button.setVisible(false);
    os16Button.setVisible(false);
    os32Button.setVisible(false);

    scopeOutButton.setButtonText("OUT");
    scopeInOutButton.setButtonText("IN/OUT");
    scopeDeltaButton.setButtonText("DELTA");
    scopePeaksButton.setButtonText("PEAKS");
    scopeOutButton.setTooltip("Show only the processed output waveform.");
    scopeInOutButton.setTooltip("Overlay input, output and highlighted clipped crests.");
    scopeDeltaButton.setTooltip("Show the removed signal in the main scope.");
    scopePeaksButton.setTooltip("Show only the clipped peaks removed by the processor.");

    osOffButton.onClick = [this] { setOversamplingChoice(0); };
    os2Button.onClick = [this] { setOversamplingChoice(1); };
    os4Button.onClick = [this] { setOversamplingChoice(2); };
    os8Button.onClick = [this] { setOversamplingChoice(3); };
    os16Button.onClick = [this] { setOversamplingChoice(4); };
    os32Button.onClick = [this] { setOversamplingChoice(5); };

    scopeOutButton.onClick = [this] { setScopeChoice(0); };
    scopeInOutButton.onClick = [this] { setScopeChoice(1); };
    scopeDeltaButton.onClick = [this] { setScopeChoice(2); };
    scopePeaksButton.onClick = [this] { setScopeChoice(3); };

    inputAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "inputGain", inputSlider);
    thresholdAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "threshold", thresholdSlider);
    kneeAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "softKnee", kneeSlider);
    outputAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "outputGain", outputSlider);
    ceilingAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "ceiling", ceilingSlider);
    mixAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "mix", mixSlider);
    colorAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "color", colorSlider);
    stereoLinkAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "stereoLink", stereoLinkSlider);
    scopeZoomAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "scopeZoom", scopeZoomSlider);

    autoGainAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "autoGain", autoGainButton);
    matchAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "match", matchButton);
    dcBlockerAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "dcBlocker", dcBlockerButton);
    deltaAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "deltaListen", deltaButton);
    bypassAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "bypass", bypassButton);

    refreshPresetBox();
    updateModeButtons();
    updateOversamplingButtons();
    updateProcessingModeBox();
    updatePhaseButtons();
    updateLiftButtons();
    updateScopeButtons();
    updateABButtons();
    updateUtilityButtons();

    startTimerHz(30);
    checkLatestVersionAsync();
}

JPClipperAudioProcessorEditor::~JPClipperAudioProcessorEditor()
{
    tooltipWindow.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
    inputSlider.setLookAndFeel(nullptr);
    thresholdSlider.setLookAndFeel(nullptr);
    kneeSlider.setLookAndFeel(nullptr);
    outputSlider.setLookAndFeel(nullptr);
    ceilingSlider.setLookAndFeel(nullptr);
    mixSlider.setLookAndFeel(nullptr);
    colorSlider.setLookAndFeel(nullptr);
    stereoLinkSlider.setLookAndFeel(nullptr);
    scopeZoomSlider.setLookAndFeel(nullptr);
    spacerSlider.setLookAndFeel(nullptr);
    liftSlider.setLookAndFeel(nullptr);

    hardButton.setLookAndFeel(nullptr);
    softButton.setLookAndFeel(nullptr);
    linearPhaseButton.setLookAndFeel(nullptr);
    minPhaseButton.setLookAndFeel(nullptr);
    osOffButton.setLookAndFeel(nullptr);
    os2Button.setLookAndFeel(nullptr);
    os4Button.setLookAndFeel(nullptr);
    os8Button.setLookAndFeel(nullptr);
    os16Button.setLookAndFeel(nullptr);
    os32Button.setLookAndFeel(nullptr);
    scopeOutButton.setLookAndFeel(nullptr);
    scopeInOutButton.setLookAndFeel(nullptr);
    scopeDeltaButton.setLookAndFeel(nullptr);
    scopePeaksButton.setLookAndFeel(nullptr);
    savePresetButton.setLookAndFeel(nullptr);
    deletePresetButton.setLookAndFeel(nullptr);
    aButton.setLookAndFeel(nullptr);
    bButton.setLookAndFeel(nullptr);
    copyABButton.setLookAndFeel(nullptr);
    pasteSettingsButton.setLookAndFeel(nullptr);
    undoButton.setLookAndFeel(nullptr);
    redoButton.setLookAndFeel(nullptr);
    helpButton.setLookAndFeel(nullptr);
    autoGainButton.setLookAndFeel(nullptr);
    matchButton.setLookAndFeel(nullptr);
    dcBlockerButton.setLookAndFeel(nullptr);
    deltaButton.setLookAndFeel(nullptr);
    bypassButton.setLookAndFeel(nullptr);
    updateAvailableButton.setLookAndFeel(nullptr);
    presetBox.setLookAndFeel(nullptr);
    oversamplingBox.setLookAndFeel(nullptr);
    scopeTypeBox.setLookAndFeel(nullptr);
    scopeZoomBox.setLookAndFeel(nullptr);
    themeBox.setLookAndFeel(nullptr);
}


void JPClipperAudioProcessorEditor::setupSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel(&analogKnobLookAndFeel);
    addAndMakeVisible(slider);
}

void JPClipperAudioProcessorEditor::setupToggleButton(juce::TextButton& button)
{
    button.setClickingTogglesState(true);
    button.setLookAndFeel(&hardwareToggleButtonLookAndFeel);
    button.setTriggeredOnMouseDown(false);
    addAndMakeVisible(button);
}

void JPClipperAudioProcessorEditor::setupLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, JPTheme::accent().brighter(0.42f).withAlpha(0.86f));
    label.setFont(JPTheme::labelFont(11.6f));
    addAndMakeVisible(label);
}

void JPClipperAudioProcessorEditor::setupSmallButton(juce::TextButton& button)
{
    button.setLookAndFeel(&hardwareButtonLookAndFeel);
    button.setTriggeredOnMouseDown(false);
    addAndMakeVisible(button);
}

void JPClipperAudioProcessorEditor::setOversamplingChoice(int choiceIndex)
{
    audioProcessor.getUndoManager().beginNewTransaction("Set Oversampling");

    if (auto* p = audioProcessor.apvts.getParameter("oversampling"))
    {
        const float normalised = juce::jlimit(0.0f, 1.0f, (float)choiceIndex / 5.0f);
        p->beginChangeGesture();
        p->setValueNotifyingHost(normalised);
        p->endChangeGesture();
    }

    updateOversamplingButtons();
}

void JPClipperAudioProcessorEditor::updateOversamplingButtons()
{
    const int choice = (int)audioProcessor.apvts.getRawParameterValue("oversampling")->load();

    osOffButton.setToggleState(choice == 0, juce::dontSendNotification);
    os2Button.setToggleState(choice == 1, juce::dontSendNotification);
    os4Button.setToggleState(choice == 2, juce::dontSendNotification);
    os8Button.setToggleState(choice == 3, juce::dontSendNotification);
    os16Button.setToggleState(choice == 4, juce::dontSendNotification);
    os32Button.setToggleState(choice == 5, juce::dontSendNotification);

    if (oversamplingBox.getSelectedId() != choice + 1)
        oversamplingBox.setSelectedId(choice + 1, juce::dontSendNotification);
}


void JPClipperAudioProcessorEditor::setProcessingModeChoice(int choiceIndex)
{
    audioProcessor.getUndoManager().beginNewTransaction("Set Process Mode");

    if (auto* p = audioProcessor.apvts.getParameter("processMode"))
    {
        const float normalised = juce::jlimit(0.0f, 1.0f, (float)choiceIndex / 3.0f);
        p->beginChangeGesture();
        p->setValueNotifyingHost(normalised);
        p->endChangeGesture();
    }

    updateProcessingModeBox();
}

void JPClipperAudioProcessorEditor::updateProcessingModeBox()
{
    const int choice = (int)audioProcessor.apvts.getRawParameterValue("processMode")->load();

    if (processingModeBox.getSelectedId() != choice + 1)
        processingModeBox.setSelectedId(choice + 1, juce::dontSendNotification);
}


void JPClipperAudioProcessorEditor::setModeChoice(int choiceIndex)
{
    audioProcessor.getUndoManager().beginNewTransaction("Set Mode");

    if (auto* p = audioProcessor.apvts.getParameter("mode"))
    {
        const float normalised = choiceIndex <= 0 ? 0.0f : 1.0f;
        p->beginChangeGesture();
        p->setValueNotifyingHost(normalised);
        p->endChangeGesture();
    }

    updateModeButtons();
}

void JPClipperAudioProcessorEditor::updateModeButtons()
{
    const int choice = (int)audioProcessor.apvts.getRawParameterValue("mode")->load();

    hardButton.setToggleState(choice == 0, juce::dontSendNotification);
    softButton.setToggleState(choice == 1, juce::dontSendNotification);
}

void JPClipperAudioProcessorEditor::setPhaseChoice(int choiceIndex)
{
    audioProcessor.getUndoManager().beginNewTransaction("Set Phase");

    if (auto* p = audioProcessor.apvts.getParameter("phaseMode"))
    {
        const float normalised = choiceIndex <= 0 ? 0.0f : 1.0f;
        p->beginChangeGesture();
        p->setValueNotifyingHost(normalised);
        p->endChangeGesture();
    }

    updatePhaseButtons();
}

void JPClipperAudioProcessorEditor::updatePhaseButtons()
{
    const int choice = (int)audioProcessor.apvts.getRawParameterValue("phaseMode")->load();

    linearPhaseButton.setToggleState(choice == 0, juce::dontSendNotification);
    minPhaseButton.setToggleState(choice == 1, juce::dontSendNotification);
}

void JPClipperAudioProcessorEditor::setLiftChoice(int choiceIndex)
{
    audioProcessor.getUndoManager().beginNewTransaction("Set Lift");

    if (auto* p = audioProcessor.apvts.getParameter("liftMode"))
    {
        const float normalised = juce::jlimit(0.0f, 1.0f, (float)choiceIndex / 3.0f);
        p->beginChangeGesture();
        p->setValueNotifyingHost(normalised);
        p->endChangeGesture();
    }

    updateLiftButtons();
}

void JPClipperAudioProcessorEditor::updateLiftButtons()
{
    const int choice = (int)audioProcessor.apvts.getRawParameterValue("liftMode")->load();

    liftSlider.setValue((double)choice, juce::dontSendNotification);

    static const juce::StringArray liftNames{ "OFF", "BODY", "DENS", "PUSH" };
    liftValueLabel.setText(liftNames[juce::jlimit(0, liftNames.size() - 1, choice)], juce::dontSendNotification);
    liftValueLabel.setColour(juce::Label::textColourId, JPTheme::accent());
    liftLabel.setColour(juce::Label::textColourId, JPTheme::accent().brighter(0.42f).withAlpha(0.86f));
}

void JPClipperAudioProcessorEditor::setScopeChoice(int choiceIndex)
{
    audioProcessor.getUndoManager().beginNewTransaction("Set Scope");

    if (auto* p = audioProcessor.apvts.getParameter("scopeMode"))
    {
        const float normalised = juce::jlimit(0.0f, 1.0f, (float)choiceIndex / 3.0f);
        p->beginChangeGesture();
        p->setValueNotifyingHost(normalised);
        p->endChangeGesture();
    }

    updateScopeButtons();
}

void JPClipperAudioProcessorEditor::updateScopeButtons()
{
    const int choice = (int)audioProcessor.apvts.getRawParameterValue("scopeMode")->load();

    if (scopeTypeBox.getSelectedId() != choice + 1)
        scopeTypeBox.setSelectedId(choice + 1, juce::dontSendNotification);

    scopeOutButton.setToggleState(choice == 0, juce::dontSendNotification);
    scopeInOutButton.setToggleState(choice == 1, juce::dontSendNotification);
    scopeDeltaButton.setToggleState(choice == 2, juce::dontSendNotification);
    scopePeaksButton.setToggleState(choice == 3, juce::dontSendNotification);
}

void JPClipperAudioProcessorEditor::updateABButtons()
{
    const int activeSlot = audioProcessor.getActiveABSlot();

    aButton.setToggleState(activeSlot == 0, juce::dontSendNotification);
    bButton.setToggleState(activeSlot == 1, juce::dontSendNotification);
}

void JPClipperAudioProcessorEditor::updateUtilityButtons()
{
    undoButton.setEnabled(audioProcessor.getUndoManager().canUndo());
    redoButton.setEnabled(audioProcessor.getUndoManager().canRedo());
    updatePresetButtons();
}

void JPClipperAudioProcessorEditor::refreshPresetBox()
{
    juce::ScopedValueSetter<bool> ignore(ignorePresetBoxChange, true);

    const auto currentName = audioProcessor.getCurrentPresetName();
    const auto currentDisplayName = currentName.isNotEmpty() ? currentName.toUpperCase() : juce::String("SESSION");
    int selectedId = 0;

    presetBox.clear(juce::dontSendNotification);
    presetBox.setTextWhenNothingSelected(currentDisplayName);

    for (int i = 0; i < audioProcessor.getNumFactoryPresets(); ++i)
    {
        const auto name = audioProcessor.getFactoryPresetName(i);
        const int itemId = factoryPresetIdBase + i;
        presetBox.addItem("FACTORY: " + name.toUpperCase(), itemId);

        if (currentName == name)
            selectedId = itemId;
    }

    audioProcessor.refreshUserPresets();

    for (int i = 0; i < audioProcessor.getNumUserPresets(); ++i)
    {
        const auto name = audioProcessor.getUserPresetName(i);
        const int itemId = userPresetIdBase + i;
        presetBox.addItem("USER: " + name.toUpperCase(), itemId);

        if (currentName == name)
            selectedId = itemId;
    }

    if (selectedId != 0)
        presetBox.setSelectedId(selectedId, juce::dontSendNotification);

    updatePresetButtons();
}

int JPClipperAudioProcessorEditor::getSelectedUserPresetIndex() const
{
    const int selectedId = presetBox.getSelectedId();

    if (selectedId < userPresetIdBase)
        return -1;

    const int userIndex = selectedId - userPresetIdBase;
    return juce::isPositiveAndBelow(userIndex, audioProcessor.getNumUserPresets()) ? userIndex : -1;
}

void JPClipperAudioProcessorEditor::updatePresetButtons()
{
    deletePresetButton.setEnabled(getSelectedUserPresetIndex() >= 0);
}

void JPClipperAudioProcessorEditor::loadSelectedPreset()
{
    if (ignorePresetBoxChange)
        return;

    const int selectedId = presetBox.getSelectedId();
    bool loaded = false;

    if (selectedId >= userPresetIdBase)
    {
        loaded = audioProcessor.loadUserPreset(selectedId - userPresetIdBase);
    }
    else if (selectedId >= factoryPresetIdBase)
    {
        audioProcessor.loadFactoryPreset(selectedId - factoryPresetIdBase);
        loaded = true;
    }

    if (!loaded)
        return;

    refreshPresetBox();
    updateModeButtons();
    updateOversamplingButtons();
    updateProcessingModeBox();
    updatePhaseButtons();
    updateLiftButtons();
    updateScopeButtons();
    updateABButtons();
    updateUtilityButtons();
}

void JPClipperAudioProcessorEditor::showSavePresetDialog()
{
    auto* window = new juce::AlertWindow("Save User Preset",
        "Enter a name for the current settings.",
        juce::AlertWindow::NoIcon,
        this);

    window->setLookAndFeel(&jpOverlayLookAndFeel);
    window->setColour(juce::AlertWindow::backgroundColourId, JPTheme::band());
    window->setColour(juce::AlertWindow::textColourId, JPTheme::accent().brighter(0.65f));
    window->setColour(juce::AlertWindow::outlineColourId, JPTheme::accent().brighter(0.45f).withAlpha(0.52f));
    window->setColour(juce::TextEditor::backgroundColourId, JPTheme::accentDeep());
    window->setColour(juce::TextEditor::textColourId, JPTheme::accent().brighter(0.65f));
    window->setColour(juce::TextEditor::outlineColourId, JPTheme::accent().brighter(0.45f).withAlpha(0.55f));
    window->setColour(juce::TextEditor::focusedOutlineColourId, JPTheme::cyan());

    window->addTextEditor("presetName", audioProcessor.getCurrentPresetName(), "Name:");
    if (auto* presetEditor = window->getTextEditor("presetName"))
    {
        presetEditor->setLookAndFeel(&jpOverlayLookAndFeel);
        presetEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colour(2, 58, 88));
        presetEditor->setColour(juce::TextEditor::textColourId, JPTheme::accent().brighter(0.65f));
        presetEditor->setColour(juce::TextEditor::highlightColourId, JPTheme::cyan().withAlpha(0.35f));
        presetEditor->setColour(juce::TextEditor::highlightedTextColourId, JPTheme::accent().brighter(0.80f));
        presetEditor->setColour(juce::TextEditor::outlineColourId, JPTheme::accent().brighter(0.45f).withAlpha(0.55f));
        presetEditor->setColour(juce::TextEditor::focusedOutlineColourId, JPTheme::cyan());
        presetEditor->setIndents(8, 5);
        presetEditor->setFont(JPTheme::titleFont(14.0f));
    }
    window->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    juce::Component::SafePointer<JPClipperAudioProcessorEditor> safeThis(this);

    window->enterModalState(true,
        juce::ModalCallbackFunction::create([safeThis, window](int result)
            {
                if (auto* presetEditor = window->getTextEditor("presetName"))
                    presetEditor->setLookAndFeel(nullptr);
                window->setLookAndFeel(nullptr);
                std::unique_ptr<juce::AlertWindow> windowDeleter(window);

                if (safeThis == nullptr || result != 1)
                    return;

                const auto name = window->getTextEditorContents("presetName").trim();

                if (name.isEmpty())
                    return;

                if (safeThis->audioProcessor.saveUserPreset(name))
                {
                    safeThis->refreshPresetBox();
                    safeThis->updateUtilityButtons();
                }
            }),
        false);
}

void JPClipperAudioProcessorEditor::deleteSelectedUserPreset()
{
    const int userIndex = getSelectedUserPresetIndex();

    if (userIndex < 0)
        return;

    const auto presetName = audioProcessor.getUserPresetName(userIndex);
    juce::Component::SafePointer<JPClipperAudioProcessorEditor> safeThis(this);

    juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon,
        "Delete User Preset",
        "Delete \"" + presetName + "\"?",
        "Delete",
        "Cancel",
        this,
        juce::ModalCallbackFunction::create([safeThis, userIndex](int result)
            {
                if (safeThis == nullptr || result == 0)
                    return;

                if (safeThis->audioProcessor.deleteUserPreset(userIndex))
                {
                    safeThis->refreshPresetBox();
                    safeThis->updateUtilityButtons();
                }
            }));
}


void JPClipperAudioProcessorEditor::checkLatestVersionAsync()
{
    const auto localVersion = getLocalPluginVersionString();
    juce::Component::SafePointer<JPClipperAudioProcessorEditor> safeThis(this);

    juce::Thread::launch(juce::Thread::Priority::low, [safeThis, localVersion]
        {
            int statusCode = 0;
            juce::String response;

            auto stream = juce::URL(versionManifestUrl).createInputStream(
                juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                    .withConnectionTimeoutMs(2500)
                    .withNumRedirectsToFollow(3)
                    .withStatusCode(&statusCode)
                    .withExtraHeaders("Accept: application/json\r\nUser-Agent: JPClipper/" + localVersion + "\r\n"));

            if (stream != nullptr && (statusCode == 0 || (statusCode >= 200 && statusCode < 300)))
                response = stream->readEntireStreamAsString();

            if (response.isEmpty())
                return;

            juce::var parsed;
            const auto result = juce::JSON::parse(response, parsed);

            if (!result.wasOk() || !parsed.isObject())
                return;

            const auto latestVersion = getLatestVersionFromManifest(parsed);

            if (latestVersion.isEmpty() || !isRemoteVersionNewer(latestVersion, localVersion))
                return;

            const auto updateUrl = getUpdateUrlFromManifest(parsed);

            juce::MessageManager::callAsync([safeThis, latestVersion, updateUrl]
                {
                    if (safeThis != nullptr)
                        safeThis->showUpdateAvailableBadge(latestVersion, updateUrl);
                });
        });
}

void JPClipperAudioProcessorEditor::setThemeVariant(int newVariant)
{
    currentThemeVariant = juce::jlimit(0, 6, newVariant);
    JPTheme::setVariant(currentThemeVariant);
    updateThemeButtons();

    footerLink.setColour(juce::HyperlinkButton::textColourId, JPTheme::accent().brighter(0.65f));

    const auto versionColour = latestAvailableVersion.isNotEmpty()
        ? JPTheme::red().brighter(0.12f).withAlpha(0.96f)
        : JPTheme::accent().brighter(0.25f).withAlpha(0.92f);
    versionLabel.setColour(juce::Label::textColourId, versionColour);

    liftValueLabel.setColour(juce::Label::textColourId, JPTheme::accent());

    for (auto* label : { &inputLabel, &thresholdLabel, &kneeLabel, &outputLabel, &ceilingLabel,
        &mixLabel, &colorLabel, &stereoLinkLabel, &scopeZoomLabel, &autoGainLabel, &matchLabel,
        &matchAmountLabel, &dcBlockerLabel, &deltaLabel, &bypassLabel, &modeLabel,
        &processingModeLabel, &liftLabel, &oversamplingLabel, &phaseLabel, &scopeLabel })
    {
        if (label != nullptr)
            label->setColour(juce::Label::textColourId, JPTheme::accent().brighter(0.42f).withAlpha(0.86f));
    }

    for (auto* valueLabel : { &inputValueLabel, &thresholdValueLabel, &kneeValueLabel,
        &outputValueLabel, &ceilingValueLabel, &mixValueLabel, &colorValueLabel,
        &stereoLinkValueLabel, &scopeZoomValueLabel })
    {
        if (valueLabel != nullptr)
            valueLabel->setColour(juce::Label::textColourId, JPTheme::accent().brighter(0.72f).withAlpha(0.94f));
    }

    liftValueLabel.setColour(juce::Label::textColourId, JPTheme::accent());

    auto applyComboTheme = [](juce::ComboBox& box)
        {
            box.setColour(juce::ComboBox::backgroundColourId, JPTheme::accentDeep().withAlpha(0.78f));
            box.setColour(juce::ComboBox::outlineColourId, JPTheme::accent().brighter(0.45f).withAlpha(0.48f));
            box.setColour(juce::ComboBox::textColourId, JPTheme::accent().brighter(0.65f));
            box.setColour(juce::ComboBox::arrowColourId, JPTheme::accent().brighter(0.55f));
            box.setColour(juce::PopupMenu::backgroundColourId, JPTheme::accentDeep());
            box.setColour(juce::PopupMenu::textColourId, JPTheme::accent().brighter(0.65f));
        };

    applyComboTheme(presetBox);
    applyComboTheme(oversamplingBox);
    applyComboTheme(processingModeBox);
    applyComboTheme(scopeTypeBox);
    applyComboTheme(scopeZoomBox);
    applyComboTheme(themeBox);

    repaint();
    inputMeter.repaint();
    outputMeter.repaint();
    waveformDisplay.repaint();

    for (auto* child : getChildren())
        if (child != nullptr)
            child->repaint();
}

void JPClipperAudioProcessorEditor::updateThemeButtons()
{
    const int selectedId = currentThemeVariant + 1;
    if (themeBox.getSelectedId() != selectedId)
        themeBox.setSelectedId(selectedId, juce::dontSendNotification);
}

void JPClipperAudioProcessorEditor::showUpdateAvailableBadge(const juce::String& latestVersion,
    const juce::String& updateUrl)
{
    latestAvailableVersion = latestVersion;
    latestVersionUrl = updateUrl;

    // Cleaner update indication: no badge. Keep the installed version visible,
    // but switch it to red when a newer build is available.
    updateAvailableButton.setVisible(false);
    updateAvailableButton.setToggleState(false, juce::dontSendNotification);

    versionLabel.setColour(juce::Label::textColourId, JPTheme::red().brighter(0.12f).withAlpha(0.96f));
    versionLabel.setTooltip("Installed: " + getLocalPluginVersionString()
        + "\nAvailable: " + latestVersion
        + (latestVersionUrl.isNotEmpty() ? "\n" + latestVersionUrl : juce::String()));
    versionLabel.repaint();
}


void JPClipperAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title)
{
    auto clipped = area.reduced(0.0f);
    g.setColour(JPTheme::panel2().withAlpha(0.96f));
    g.fillRoundedRectangle(clipped, 9.0f);

    g.setColour(JPTheme::line().withAlpha(0.42f));
    g.drawRoundedRectangle(clipped.reduced(0.5f), 9.0f, 1.0f);

    g.setColour(juce::Colour(255, 255, 255).withAlpha(0.04f));
    g.drawLine(clipped.getX() + 12.0f, clipped.getY() + 4.0f,
        clipped.getRight() - 12.0f, clipped.getY() + 4.0f, 1.0f);

    if (title.isNotEmpty())
    {
        g.setColour(JPTheme::dimText());
        g.setFont(JPTheme::labelFont(12.0f));
        g.drawText(title, clipped.removeFromTop(34.0f).reduced(16.0f, 0.0f), juce::Justification::centredLeft);

        g.setColour(JPTheme::line().withAlpha(0.36f));
        g.drawLine(clipped.getX() + 14.0f, clipped.getY(),
            clipped.getRight() - 14.0f, clipped.getY(), 1.0f);
    }
}

void JPClipperAudioProcessorEditor::drawHeader(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto titleArea = area.reduced(22, 0).removeFromLeft(270).reduced(0, 7);
    auto brand = titleArea.removeFromTop(20);
    auto product = titleArea.removeFromTop(34);

    g.setColour(JPTheme::accent().brighter(0.58f));
    g.setFont(JPTheme::titleFont(31.0f));
    const float productW = getTextWidth(g.getCurrentFont(), "JP|CLIPPER");
    const float logoW = juce::jlimit(132.0f, 178.0f, productW);
    auto logoProduct = product.withWidth((int)std::ceil(logoW) + 2);
    g.drawFittedText("JP|CLIPPER", logoProduct, juce::Justification::centredLeft, 1, 0.92f);

    g.setColour(JPTheme::accent().brighter(0.42f).withAlpha(0.88f));
    g.setFont(JPTheme::titleFont(13.5f));
    const juce::String brandText("JUICY PLUGINS");
    auto logoBrand = brand.withWidth(logoProduct.getWidth()).toFloat();
    const float charCount = (float)brandText.length();
    const float naturalW = getTextWidth(g.getCurrentFont(), brandText);
    const float extraTracking = charCount > 1.0f ? juce::jmax(0.0f, (logoBrand.getWidth() - naturalW) / (charCount - 1.0f)) : 0.0f;
    float x = logoBrand.getX();
    for (int i = 0; i < brandText.length(); ++i)
    {
        const juce::String ch = brandText.substring(i, i + 1);
        g.drawText(ch, juce::Rectangle<float>(x, logoBrand.getY(), 18.0f, logoBrand.getHeight()), juce::Justification::centredLeft, false);
        x += getTextWidth(g.getCurrentFont(), ch) + extraTracking;
    }
}




void JPClipperAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(JPTheme::bg());

    juce::Graphics::ScopedSaveState state(g);
    g.addTransform(getContentTransform());

    auto bounds = juce::Rectangle<int>(0, 0, defaultEditorWidth, defaultEditorHeight).reduced(8);
    auto header = bounds.removeFromTop(66);
    drawHeader(g, header);

    const auto displayBand = juce::Rectangle<float>(0.0f, (float)header.getBottom() + 4.0f,
        (float)defaultEditorWidth, 258.0f);
    const auto footer = juce::Rectangle<float>(0.0f, (float)defaultEditorHeight - 34.0f,
        (float)defaultEditorWidth, 34.0f);

    g.setColour(JPTheme::band().withAlpha(0.98f));
    g.fillRect(displayBand);

    g.setColour(JPTheme::accent().brighter(0.68f).withAlpha(0.05f));
    g.drawLine(displayBand.getX(), displayBand.getY() + 2.0f, displayBand.getRight(), displayBand.getY() + 2.0f, 1.0f);
    g.setColour(JPTheme::accentDeep().darker(0.15f).withAlpha(0.20f));
    g.drawLine(displayBand.getX(), displayBand.getBottom(), displayBand.getRight(), displayBand.getBottom(), 1.0f);

    g.setColour(JPTheme::band().withAlpha(0.98f));
    g.fillRect(footer);
    g.setColour(JPTheme::accent().brighter(0.45f).withAlpha(0.12f));
    g.drawLine(footer.getX(), footer.getY(), footer.getRight(), footer.getY(), 1.0f);
}

float JPClipperAudioProcessorEditor::getUiScale() const
{
    const float widthScale = (float)getWidth() / (float)defaultEditorWidth;
    const float heightScale = (float)getHeight() / (float)defaultEditorHeight;

    return juce::jlimit((float)minEditorWidth / (float)defaultEditorWidth,
        (float)maxEditorWidth / (float)defaultEditorWidth,
        juce::jmin(widthScale, heightScale));
}

juce::AffineTransform JPClipperAudioProcessorEditor::getContentTransform() const
{
    const float scale = getUiScale();
    const float contentWidth = (float)defaultEditorWidth * scale;
    const float contentHeight = (float)defaultEditorHeight * scale;
    const float offsetX = ((float)getWidth() - contentWidth) * 0.5f;
    const float offsetY = ((float)getHeight() - contentHeight) * 0.5f;

    return juce::AffineTransform::scale(scale).translated(offsetX, offsetY);
}

void JPClipperAudioProcessorEditor::applyGlobalScaleTransform()
{
    const auto transform = getContentTransform();

    auto apply = [transform](juce::Component& component)
        {
            component.setTransform(transform);
        };

    apply(inputMeter);
    apply(outputMeter);
    apply(waveformDisplay);
    apply(presetBox);
    apply(oversamplingBox);
    apply(processingModeBox);
    apply(scopeTypeBox);
    apply(scopeZoomBox);
    apply(footerLink);
    apply(versionLabel);
    apply(updateAvailableButton);
    apply(themeBox);
    apply(savePresetButton);
    apply(deletePresetButton);
    apply(aButton);
    apply(bButton);
    apply(copyABButton);
    apply(pasteSettingsButton);
    apply(undoButton);
    apply(redoButton);
    apply(helpButton);
    apply(inputSlider);
    apply(thresholdSlider);
    apply(kneeSlider);
    apply(outputSlider);
    apply(ceilingSlider);
    apply(mixSlider);
    apply(colorSlider);
    apply(stereoLinkSlider);
    apply(scopeZoomSlider);
    apply(spacerSlider);
    apply(spacerLabel);
    apply(spacerValueLabel);
    apply(autoGainButton);
    apply(matchButton);
    apply(dcBlockerButton);
    apply(deltaButton);
    apply(bypassButton);
    apply(hardButton);
    apply(softButton);
    apply(linearPhaseButton);
    apply(minPhaseButton);
    apply(liftSlider);
    apply(osOffButton);
    apply(os2Button);
    apply(os4Button);
    apply(os8Button);
    apply(os16Button);
    apply(os32Button);
    apply(scopeOutButton);
    apply(scopeInOutButton);
    apply(scopeDeltaButton);
    apply(scopePeaksButton);
    apply(inputLabel);
    apply(thresholdLabel);
    apply(kneeLabel);
    apply(outputLabel);
    apply(ceilingLabel);
    apply(mixLabel);
    apply(colorLabel);
    apply(stereoLinkLabel);
    apply(scopeZoomLabel);
    apply(autoGainLabel);
    apply(matchLabel);
    apply(matchAmountLabel);
    apply(dcBlockerLabel);
    apply(deltaLabel);
    apply(bypassLabel);
    apply(modeLabel);
    apply(processingModeLabel);
    apply(liftLabel);
    apply(liftValueLabel);
    apply(oversamplingLabel);
    apply(phaseLabel);
    apply(scopeLabel);
    apply(inputValueLabel);
    apply(thresholdValueLabel);
    apply(kneeValueLabel);
    apply(outputValueLabel);
    apply(ceilingValueLabel);
    apply(mixValueLabel);
    apply(colorValueLabel);
    apply(stereoLinkValueLabel);
    apply(scopeZoomValueLabel);
}

void JPClipperAudioProcessorEditor::resized()
{
    audioProcessor.setEditorSize(getWidth(), getHeight());
    applyGlobalScaleTransform();

    auto bounds = juce::Rectangle<int>(0, 0, defaultEditorWidth, defaultEditorHeight).reduced(8);
    auto header = bounds.removeFromTop(66);

    auto headerControls = header.reduced(22, 14);
    headerControls.removeFromLeft(300);

    const int headerIconSize = 34;
    auto takeHeaderIcon = [&](juce::Rectangle<int>& row, int width)
        {
            auto slot = row.removeFromLeft(width);
            const int size = juce::jmin(slot.getWidth(), headerIconSize);
            return juce::Rectangle<int>(slot.getCentreX() - size / 2,
                slot.getCentreY() - size / 2,
                size,
                size);
        };

    auto rightHeader = headerControls.removeFromRight(650);
    presetBox.setBounds(rightHeader.removeFromLeft(218));
    rightHeader.removeFromLeft(7);
    savePresetButton.setBounds(takeHeaderIcon(rightHeader, headerIconSize));
    rightHeader.removeFromLeft(7);
    deletePresetButton.setBounds(takeHeaderIcon(rightHeader, headerIconSize));
    rightHeader.removeFromLeft(10);
    aButton.setBounds(takeHeaderIcon(rightHeader, headerIconSize));
    rightHeader.removeFromLeft(7);
    bButton.setBounds(takeHeaderIcon(rightHeader, headerIconSize));
    rightHeader.removeFromLeft(10);
    copyABButton.setBounds(takeHeaderIcon(rightHeader, headerIconSize));
    rightHeader.removeFromLeft(7);
    pasteSettingsButton.setBounds(takeHeaderIcon(rightHeader, headerIconSize));
    rightHeader.removeFromLeft(12);
    undoButton.setBounds(takeHeaderIcon(rightHeader, headerIconSize));
    rightHeader.removeFromLeft(7);
    redoButton.setBounds(takeHeaderIcon(rightHeader, headerIconSize));
    rightHeader.removeFromLeft(7);
    helpButton.setBounds(takeHeaderIcon(rightHeader, headerIconSize));
    rightHeader.removeFromLeft(7);
    bypassButton.setBounds(takeHeaderIcon(rightHeader, headerIconSize));

    auto footer = juce::Rectangle<int>(0, defaultEditorHeight - 34, defaultEditorWidth, 34);
    auto centreInFooter = [&footer](int x, int width, int height)
        {
            return juce::Rectangle<int>(x, 0, width, height)
                .withCentre({ x + width / 2, footer.getCentreY() });
        };

    footerLink.setBounds(centreInFooter(28, 132, 24));

    const int themeBoxWidth = 112;
    themeBox.setBounds(centreInFooter(footer.getCentreX() - themeBoxWidth / 2, themeBoxWidth, 24));

    versionLabel.setBounds(centreInFooter(footer.getRight() - 34 - 86, 86, 24));
    updateAvailableButton.setBounds(0, 0, 0, 0);

    auto displayBand = juce::Rectangle<int>(0, header.getBottom() + 4, defaultEditorWidth, 258).reduced(24, 6);
    const int sideMeterWidth = 90;
    const int displayGap = 18;
    const int inputColumnX = displayBand.getX();
    const int outputColumnX = displayBand.getRight() - sideMeterWidth;
    inputMeter.setBounds(displayBand.removeFromLeft(sideMeterWidth));
    displayBand.removeFromLeft(displayGap);
    outputMeter.setBounds(displayBand.removeFromRight(sideMeterWidth));
    displayBand.removeFromRight(displayGap);
    waveformDisplay.setBounds(displayBand);
    scopeZoomBox.setBounds(displayBand.getRight() - 76, displayBand.getBottom() - 34, 70, 25);
    scopeTypeBox.setBounds(scopeZoomBox.getX() - 96, scopeZoomBox.getY(), 88, 25);

    const int knobContentY = header.getBottom() + 4 + 258 + 5;
    auto content = juce::Rectangle<int>(18, knobContentY, defaultEditorWidth - 36, footer.getY() - knobContentY - 5);
    auto knobBand = content.removeFromTop(160);

    constexpr int bigKnob = 104;
    constexpr int ioKnob = bigKnob;
    constexpr int smallKnob = 84;
    const int labelH = 17;
    const int bigLabelY = knobBand.getY() + 4;
    const int bigSliderY = knobBand.getY() + 18;
    const int smallLabelY = knobBand.getY() + 58;
    const int smallSliderY = knobBand.getY() + 72;
    const int ioLabelY = knobBand.getY() + 18;
    const int ioSliderY = knobBand.getY() + 33;

    auto placeKnobSized = [&](juce::Slider& slider,
        juce::Label& label,
        juce::Label& valueLabel,
        int x,
        int size,
        int labelY,
        int sliderY)
        {
            const int textW = juce::jmax(size + 48, size);
            const int textX = x + (size - textW) / 2;
            label.setBounds(textX, labelY, textW, labelH);
            slider.setBounds(x, sliderY, size, size);
            valueLabel.setBounds(0, 0, 0, 0);
        };

    placeKnobSized(inputSlider, inputLabel, inputValueLabel,
        inputColumnX + (sideMeterWidth - ioKnob) / 2, ioKnob, ioLabelY, ioSliderY);

    placeKnobSized(outputSlider, outputLabel, outputValueLabel,
        outputColumnX + (sideMeterWidth - ioKnob) / 2, ioKnob, ioLabelY, ioSliderY);

    const int mainLeft = inputColumnX + sideMeterWidth + displayGap + 26;
    const int mainRight = outputColumnX - displayGap - 26;
    const int mainWidth = mainRight - mainLeft;

    const int smallGap = juce::jlimit(96, 132, (mainWidth - smallKnob * 4) / 3);
    const int smallTotal = smallKnob * 4 + smallGap * 3;
    int smallX = mainLeft + (mainWidth - smallTotal) / 2;

    const int liftX = smallX;
    const int colorX = liftX + smallKnob + smallGap;
    const int mixX = colorX + smallKnob + smallGap;
    const int stereoLinkX = mixX + smallKnob + smallGap;

    placeKnobSized(liftSlider, liftLabel, liftValueLabel, liftX, smallKnob, smallLabelY, smallSliderY);
    placeKnobSized(colorSlider, colorLabel, colorValueLabel, colorX, smallKnob, smallLabelY, smallSliderY);
    placeKnobSized(stereoLinkSlider, stereoLinkLabel, stereoLinkValueLabel, mixX, smallKnob, smallLabelY, smallSliderY);
    placeKnobSized(mixSlider, mixLabel, mixValueLabel, stereoLinkX, smallKnob, smallLabelY, smallSliderY);

    const int liftCentre = liftX + smallKnob / 2;
    const int colorCentre = colorX + smallKnob / 2;
    const int mixCentre = mixX + smallKnob / 2;
    const int stereoLinkCentre = stereoLinkX + smallKnob / 2;

    // The three primary knobs sit exactly in the spaces between the four lower
    // knobs. This keeps the visual rhythm locked and lets the controls grow
    // without feeling crowded.
    const int thresholdCentre = (liftCentre + colorCentre) / 2;
    const int kneeCentre = (colorCentre + mixCentre) / 2;
    const int ceilingCentre = (mixCentre + stereoLinkCentre) / 2;

    placeKnobSized(thresholdSlider, thresholdLabel, thresholdValueLabel, thresholdCentre - bigKnob / 2, bigKnob, bigLabelY, bigSliderY);
    placeKnobSized(kneeSlider, kneeLabel, kneeValueLabel, kneeCentre - bigKnob / 2, bigKnob, bigLabelY, bigSliderY);
    placeKnobSized(ceilingSlider, ceilingLabel, ceilingValueLabel, ceilingCentre - bigKnob / 2, bigKnob, bigLabelY, bigSliderY);

    scopeZoomSlider.setBounds(0, 0, 0, 0);
    scopeZoomLabel.setBounds(0, 0, 0, 0);
    scopeZoomValueLabel.setBounds(0, 0, 0, 0);

    spacerSlider.setBounds(0, 0, 0, 0);
    spacerLabel.setBounds(0, 0, 0, 0);
    spacerValueLabel.setBounds(0, 0, 0, 0);

    content.removeFromTop(2);
    auto controls = content.reduced(2, 0);

    const int controlLabelH = 17;
    const int labelButtonGap = 5;
    const int controlButtonH = 28;
    const int controlY = controls.getY();
    const int controlButtonY = controlY + controlLabelH + labelButtonGap;
    const int groupGap = 28;

    const int buttonsTotalWidth = 78 + 94 + 84 + 66 + 118 + 88 + 88 + 122 + groupGap * 7;
    int buttonX = controls.getCentreX() - buttonsTotalWidth / 2;

    bypassLabel.setBounds(0, 0, 0, 0);

    autoGainLabel.setBounds(buttonX, controlY, 78, controlLabelH);
    autoGainButton.setBounds(buttonX, controlButtonY, 78, controlButtonH);
    buttonX += 78 + groupGap;

    matchLabel.setBounds(buttonX, controlY, 94, controlLabelH);
    matchButton.setBounds(buttonX, controlButtonY, 94, controlButtonH);
    matchAmountLabel.setBounds(0, 0, 0, 0);
    buttonX += 94 + groupGap;

    dcBlockerLabel.setBounds(buttonX, controlY, 84, controlLabelH);
    dcBlockerButton.setBounds(buttonX + 8, controlButtonY, 68, controlButtonH);
    buttonX += 84 + groupGap;

    deltaLabel.setBounds(buttonX, controlY, 66, controlLabelH);
    deltaButton.setBounds(buttonX, controlButtonY, 66, controlButtonH);
    buttonX += 66 + groupGap;

    modeLabel.setBounds(buttonX, controlY, 118, controlLabelH);
    hardButton.setBounds(buttonX, controlButtonY, 56, controlButtonH);
    softButton.setBounds(buttonX + 62, controlButtonY, 56, controlButtonH);
    buttonX += 118 + groupGap;

    processingModeLabel.setBounds(buttonX, controlY, 88, controlLabelH);
    processingModeBox.setBounds(buttonX, controlButtonY, 88, controlButtonH);
    buttonX += 88 + groupGap;

    oversamplingLabel.setBounds(buttonX, controlY, 88, controlLabelH);
    oversamplingBox.setBounds(buttonX, controlButtonY, 88, controlButtonH);
    buttonX += 88 + groupGap;

    phaseLabel.setBounds(buttonX, controlY, 122, controlLabelH);
    linearPhaseButton.setBounds(buttonX, controlButtonY, 58, controlButtonH);
    minPhaseButton.setBounds(buttonX + 64, controlButtonY, 58, controlButtonH);

    osOffButton.setBounds(0, 0, 0, 0);
    os2Button.setBounds(0, 0, 0, 0);
    os4Button.setBounds(0, 0, 0, 0);
    os8Button.setBounds(0, 0, 0, 0);
    os16Button.setBounds(0, 0, 0, 0);
    os32Button.setBounds(0, 0, 0, 0);
}

void JPClipperAudioProcessorEditor::timerCallback()
{
    std::array<float, JPClipperAudioProcessor::waveformSize> inputMinData;
    std::array<float, JPClipperAudioProcessor::waveformSize> inputMaxData;
    std::array<float, JPClipperAudioProcessor::waveformSize> clippedMinData;
    std::array<float, JPClipperAudioProcessor::waveformSize> clippedMaxData;
    std::array<float, JPClipperAudioProcessor::waveformSize> removedMinData;
    std::array<float, JPClipperAudioProcessor::waveformSize> removedMaxData;

    audioProcessor.copyWaveformData(inputMinData, inputMaxData,
        clippedMinData, clippedMaxData,
        removedMinData, removedMaxData);

    const float inputGainDb = audioProcessor.apvts.getRawParameterValue("inputGain")->load();
    const float thresholdDb = audioProcessor.apvts.getRawParameterValue("threshold")->load();
    const float softKnee = audioProcessor.apvts.getRawParameterValue("softKnee")->load();
    const float outputGainDb = audioProcessor.apvts.getRawParameterValue("outputGain")->load();
    const float ceilingDb = audioProcessor.apvts.getRawParameterValue("ceiling")->load();
    const float mix = audioProcessor.apvts.getRawParameterValue("mix")->load();
    const float color = audioProcessor.apvts.getRawParameterValue("color")->load();
    const float stereoLink = audioProcessor.apvts.getRawParameterValue("stereoLink")->load();
    const float scopeZoom = audioProcessor.apvts.getRawParameterValue("scopeZoom")->load();
    const float thresholdLinear = juce::Decibels::decibelsToGain(thresholdDb);
    const bool deltaListenEnabled = audioProcessor.apvts.getRawParameterValue("deltaListen")->load() > 0.5f;
    const int scopeMode = (int)audioProcessor.apvts.getRawParameterValue("scopeMode")->load();

    updateModeButtons();
    updateOversamplingButtons();
    updateProcessingModeBox();
    updatePhaseButtons();
    updateLiftButtons();
    updateScopeButtons();
    updateABButtons();
    updateUtilityButtons();

    inputLeftMeterSmoothed = inputLeftMeterSmoothed * 0.86f + audioProcessor.getInputLeftLevel() * 0.14f;
    inputRightMeterSmoothed = inputRightMeterSmoothed * 0.86f + audioProcessor.getInputRightLevel() * 0.14f;
    outputLeftMeterSmoothed = outputLeftMeterSmoothed * 0.86f + audioProcessor.getOutputLeftLevel() * 0.14f;
    outputRightMeterSmoothed = outputRightMeterSmoothed * 0.86f + audioProcessor.getOutputRightLevel() * 0.14f;
    inputRmsDbSmoothed = inputRmsDbSmoothed * 0.92f + audioProcessor.getInputRmsDb() * 0.08f;
    outputRmsDbSmoothed = outputRmsDbSmoothed * 0.92f + audioProcessor.getOutputRmsDb() * 0.08f;
    clipReductionDbSmoothed = clipReductionDbSmoothed * 0.85f + audioProcessor.getClipReductionDb() * 0.15f;

    inputValueLabel.setText(juce::String::formatted("%+.1f dB", inputGainDb), juce::dontSendNotification);
    thresholdValueLabel.setText(juce::String::formatted("%.1f dB", thresholdDb), juce::dontSendNotification);
    kneeValueLabel.setText(juce::String::formatted("%.0f%%", softKnee), juce::dontSendNotification);
    outputValueLabel.setText(juce::String::formatted("%+.1f dB", outputGainDb), juce::dontSendNotification);
    ceilingValueLabel.setText(juce::String::formatted("%.1f dB", ceilingDb), juce::dontSendNotification);
    mixValueLabel.setText(juce::String::formatted("%.0f%%", mix), juce::dontSendNotification);
    colorValueLabel.setText(juce::String::formatted("%.0f%%", color), juce::dontSendNotification);
    stereoLinkValueLabel.setText(juce::String::formatted("%.0f%%", stereoLink), juce::dontSendNotification);
    scopeZoomValueLabel.setText(juce::String::formatted("%.0fX", scopeZoom), juce::dontSendNotification);
    const int scopeZoomChoice = juce::jlimit(1, 8, (int)std::round(scopeZoom));
    if (scopeZoomBox.getSelectedId() != scopeZoomChoice)
        scopeZoomBox.setSelectedId(scopeZoomChoice, juce::dontSendNotification);

    inputMeter.setLevels(inputLeftMeterSmoothed, inputRightMeterSmoothed, inputRmsDbSmoothed);
    outputMeter.setLevels(outputLeftMeterSmoothed,
        outputRightMeterSmoothed,
        outputRmsDbSmoothed,
        audioProcessor.getOutputTruePeakDb(),
        audioProcessor.getOutputIspActive());

    waveformDisplay.setData(inputMinData, inputMaxData,
        clippedMinData, clippedMaxData,
        removedMinData, removedMaxData,
        thresholdLinear, clipReductionDbSmoothed,
        outputRmsDbSmoothed,
        audioProcessor.getOutputShortTermLufs(),
        audioProcessor.getOutputTruePeakDb(),
        ceilingDb,
        deltaListenEnabled,
        scopeMode,
        (int)audioProcessor.apvts.getRawParameterValue("processMode")->load());
}
