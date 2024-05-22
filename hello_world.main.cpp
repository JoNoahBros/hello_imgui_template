#include "hello_imgui/hello_imgui.h"
#include <emscripten/fetch.h>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include "source_implot/implot.h"
#include "source_implot/implot_internal.h"
#include <vector>
#include <stdio.h>
#include "nlohmann/json.hpp"
#include "hello_imgui/renderer_backend_options.h"
#include "hello_imgui/icons_font_awesome_6.h"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include "regex"
#include <string>

static char szTestText[200];
// static char responseAPI[200];
bool _flag_start_heating = false;
bool _flag_start_logging = false;
int _cmd_start_logging = 0;
float ua, ur, uref, ubat, pt1000;
int slideVal = 24;
double lastTime = 0.0;
int chip_cmd = 0;
std::string responseText = "Default";
bool heatingOn = false;
bool loggingOn = false;
bool chipReq = false;
// Define a callback type
using ResponseHandler = std::function<void(const std::string &)>;
std::string responseAPI;
std::vector<int> parsedValues;
std::string chipResponseAddress;
std::string chipResponseValues;
int chipRespAddressInt;
int chipRespValuesInt;

const auto regEx = std::regex(R"(\[(\d*),(\d*)\])");

std::optional<std::pair<int, int>> RegExFun(const std::string &input)
{

    std::smatch m;
    if (std::regex_search(input, m, regEx))
    {
        if (m.size() == 3)
        {

            return std::make_pair(
                std::stoi(m[1].str()),
                std::stoi(m[2].str())

            );
        }
    }
    return std::nullopt;
}

// Struct to hold fetch context
struct FetchContext
{
    void (*callback)(const std::string &);
};
struct ScrollingBuffer
{
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer(int max_size = 2000)
    {
        MaxSize = max_size;
        Offset = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y)
    {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x, y));
        else
        {
            Data[Offset] = ImVec2(x, y);
            Offset = (Offset + 1) % MaxSize;
        }
    }
    void Erase()
    {
        if (Data.size() > 0)
        {
            Data.shrink(0);
            Offset = 0;
        }
    }
};
struct RollingBuffer
{
    float Span;
    ImVector<ImVec2> Data;
    RollingBuffer()
    {
        Span = 10.0f;
        Data.reserve(2000);
    }
    void AddPoint(float x, float y)
    {
        float xmod = fmodf(x, Span);
        if (!Data.empty() && xmod < Data.back().x)
            Data.shrink(0);
        Data.push_back(ImVec2(xmod, y));
    }
};
struct AppState
{
    // MyAppSettings myAppSettings; // This values will be stored in the application settings

    HelloImGui::FontDpiResponsive *TitleFont;
    HelloImGui::FontDpiResponsive *ColorFont;
    HelloImGui::FontDpiResponsive *EmojiFont;
    HelloImGui::FontDpiResponsive *LargeIconFont;
    HelloImGui::FontDpiResponsive *ButtonFont;
    HelloImGui::FontDpiResponsive *HeaderFont;
    HelloImGui::FontDpiResponsive *StatusFont;
    HelloImGui::FontDpiResponsive *PlotFont;
    HelloImGui::FontDpiResponsive *CustomFont5;
    HelloImGui::FontDpiResponsive *SFDBlack;
    HelloImGui::FontDpiResponsive *SFDBold;
    HelloImGui::FontDpiResponsive *SFDHeavy;
    HelloImGui::FontDpiResponsive *SFDLight;
    HelloImGui::FontDpiResponsive *SFDMedium;
    HelloImGui::FontDpiResponsive *SFDRegular;
    HelloImGui::FontDpiResponsive *SFDSemibold;
    HelloImGui::FontDpiResponsive *SFDThin;
    HelloImGui::FontDpiResponsive *SFDUltralight;
    HelloImGui::FontDpiResponsive *SFTBold;
    HelloImGui::FontDpiResponsive *SFTBoldItalic;
    HelloImGui::FontDpiResponsive *SFTHeavy;
    HelloImGui::FontDpiResponsive *SFTHeavyItalic;
    HelloImGui::FontDpiResponsive *SFTLight;
    HelloImGui::FontDpiResponsive *SFTLightItalic;
    HelloImGui::FontDpiResponsive *SFTMedium;
    HelloImGui::FontDpiResponsive *SFTMediumItalic;
    HelloImGui::FontDpiResponsive *SFTRegular;
    HelloImGui::FontDpiResponsive *SFTRegularItalic;
    HelloImGui::FontDpiResponsive *SFTRSemibold;
    HelloImGui::FontDpiResponsive *SFTSemiboldItalic;
};
struct ChipButton
{
    const char *name;
    int value;
};
struct Colors
{
    static const ImVec4 Red;
    static const ImVec4 Green;
    static const ImVec4 Blue;
    static const ImVec4 Yellow;
    static const ImVec4 Cyan;
    static const ImVec4 Magenta;
    static const ImVec4 White;
    static const ImVec4 Black;
};
const std::array<ChipButton, 9> chipButtons = {{
    {"IDENT_REG_REQUEST", 0},    // (72, 0)
    {"DIAG_REG_REQUEST", 1},     // (120, 0)
    {"REG1_INIT_REQUEST", 2},    // (108, 0)
    {"REG1_WRITE_REQUEST", 3},   // (86, 0)
    {"REG1_CALIBRATE_MODE", 4},  // (86, 157)
    {"REG1_NORMAL_MODE_V8", 5},  // (86, 136)
    {"REG1_MODE_NORMAL_V17", 6}, // (86, 137)
    {"REG2_INIT_REQUEST", 7},    // (126, 0)
    {"REG2_WRITE_REQUEST", 8}    // (90, 0)
}};
void downloadSucceeded(emscripten_fetch_t *fetch)
{
    // Retrieve the context
    FetchContext *context = static_cast<FetchContext *>(fetch->userData);

    // Copy the data to responseAPI
    responseAPI.assign(fetch->data, fetch->numBytes);

    // Call the appropriate callback with the data
    if (context && context->callback)
    {
        context->callback(responseAPI);
    }

    // Cleanup fetch resources
    emscripten_fetch_close(fetch);
    delete context;
}
void downloadFailed(emscripten_fetch_t *fetch)
{
    printf("Download failed for URL: %s\n", fetch->url);
    // Cleanup fetch resources
    emscripten_fetch_close(fetch);
}
void startFetch(const std::string &url, void (*callback)(const std::string &))
{
    // Create a context with the appropriate handler
    FetchContext *context = new FetchContext{callback};

    // Initialize fetch attributes
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;
    attr.userData = context;

    printf("Requesting URL: %s\n", url.c_str());
    emscripten_fetch(&attr, url.c_str());
}
void handleSetHeating(const std::string &data)
{
    printf("Set heating response: %s\n", data.c_str());
    responseAPI = data;
    // You can parse the response here if needed
}
void setHeating(bool value)
{
    const std::string url = "api/setHeating?h=" + std::string(value ? "true" : "false");
    startFetch(url, handleSetHeating);
}
void handleAnalogValues(const std::string &data)
{
    std::stringstream dataStream(data);
    const auto read = [&dataStream](float &value)
    {
        std::string line;
        getline(dataStream, line, ' ');
        printf("Read line %s\n", line.c_str());
        sscanf(line.c_str(), "%f", &value);
    };

    read(ua);
    read(ur);
    read(uref);
    read(ubat);
    read(pt1000);

    printf("UA: %f, UR: %f, UREF: %f, UBAT: %f, PT1000: %f\n", ua, ur, uref, ubat, pt1000);
}
void handleSetLogging(const std::string &data)
{
    responseAPI = data;
}
void handleChipRequest(const std::string &data)
{
    // printf("Chip response: %s\n", data.c_str());

    if (const auto responseChip = RegExFun(data))
    {
        chipRespAddressInt = responseChip->first;
        chipRespValuesInt = responseChip->second;
        chipResponseAddress = std::to_string(chipRespAddressInt);
        chipResponseValues = std::to_string(chipRespValuesInt);
    }
    // responseAPI = chipResponseAddress;
}
void handleChipID(const std::string &data)
{
    printf("Chip ID: %s\n", data.c_str());
    responseAPI = data;
}
void handleSlider(const std::string &data)
{
    printf("Setpoint: %s\n", data.c_str());
    responseAPI = data;
}
void setLogging(bool value)
{
    // Construct the URL with the logging value
    const std::string url = "api/setLogging?l=" + std::string(value ? "true" : "false");
    startFetch(url, handleSetLogging);
}
void sliderValue(int value)
{
    const std::string url = "api/setSlider?v=" + std::to_string(value);
    startFetch(url, handleSlider);
}
void ChipSend(bool flag)
{
    const std::string url = "api/cmdChip?f=" + std::string(flag ? "true" : "false");
    startFetch(url, handleChipRequest);
    // emscripten_fetch(&attr, url.c_str());
}
void ChipCommand(int value)
{
    const std::string url = "api/cmdChipCommand?v=" + std::to_string(value);
    startFetch(url, handleChipID);
}
void RealtimePlots()
{
    static ScrollingBuffer sdata1, sdata2, sdata3, sdata4, sdata5, sdata6;
    static RollingBuffer rdata1, rdata2, rdata3, rdata4, rdata5, rdata6;
    // ImVec2 mouse = ImGui::GetMousePos();
    const auto t = ImGui::GetTime();

    sdata1.AddPoint(t, ua);
    sdata2.AddPoint(t, ur);
    sdata3.AddPoint(t, uref);
    sdata4.AddPoint(t, ubat);
    sdata5.AddPoint(t, pt1000);
    rdata1.AddPoint(t, ua);
    rdata2.AddPoint(t, ur);
    rdata3.AddPoint(t, uref);
    rdata4.AddPoint(t, ubat);
    rdata5.AddPoint(t, pt1000);

    static float history = 10.0f;
    ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");
    rdata1.Span = history;
    rdata2.Span = history;
    rdata3.Span = history;
    rdata4.Span = history;
    rdata5.Span = history;

    static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

    if (ImPlot::BeginPlot("Scrolling", ImVec2(-1, 150)))
    {
        ImPlot::SetupAxes(NULL, NULL, flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 10);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
        // ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
        ImPlot::PlotLine("UA", &sdata1.Data[0].x, &sdata1.Data[0].y, sdata1.Data.size(), 0, sdata1.Offset, 2 * sizeof(float));
        ImPlot::PlotLine("UR", &sdata2.Data[0].x, &sdata2.Data[0].y, sdata2.Data.size(), 0, sdata2.Offset, 2 * sizeof(float));
        ImPlot::PlotLine("UREF", &sdata3.Data[0].x, &sdata3.Data[0].y, sdata3.Data.size(), 0, sdata3.Offset, 2 * sizeof(float));
        ImPlot::PlotLine("UBAT", &sdata4.Data[0].x, &sdata4.Data[0].y, sdata4.Data.size(), 0, sdata4.Offset, 2 * sizeof(float));
        ImPlot::PlotLine("PT1000", &sdata5.Data[0].x, &sdata5.Data[0].y, sdata5.Data.size(), 0, sdata5.Offset, 2 * sizeof(float));
        ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("Rolling", ImVec2(-1, 150)))
    {
        ImPlot::SetupAxes(NULL, NULL, flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, history, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 10);
        ImPlot::PlotLine("UA", &rdata1.Data[0].x, &rdata1.Data[0].y, rdata1.Data.size(), 0, 0, 2 * sizeof(float));
        ImPlot::PlotLine("UR", &rdata2.Data[0].x, &rdata2.Data[0].y, rdata2.Data.size(), 0, 0, 2 * sizeof(float));
        ImPlot::PlotLine("UREF", &rdata3.Data[0].x, &rdata3.Data[0].y, rdata3.Data.size(), 0, 0, 2 * sizeof(float));
        ImPlot::PlotLine("UBAT", &rdata4.Data[0].x, &rdata4.Data[0].y, rdata4.Data.size(), 0, 0, 2 * sizeof(float));
        ImPlot::PlotLine("PT1000", &rdata5.Data[0].x, &rdata5.Data[0].y, rdata5.Data.size(), 0, 0, 2 * sizeof(float));
        ImPlot::EndPlot();
    }

    // if (ImPlot::BeginPlot("##Rolling", ImVec2(-1,150))) {
    //     ImPlot::SetupAxes(NULL, NULL, flags, flags);
    //     ImPlot::SetupAxisLimits(ImAxis_X1,0,history, ImGuiCond_Always);
    //     ImPlot::SetupAxisLimits(ImAxis_Y1,0,1);
    //     ImPlot::PlotLine("Mouse X", &rdata1.Data[0].x, &rdata1.Data[0].y, rdata1.Data.size(), 0, 0, 2 * sizeof(float));
    //     ImPlot::PlotLine("Mouse Y", &rdata2.Data[0].x, &rdata2.Data[0].y, rdata2.Data.size(), 0, 0, 2 * sizeof(float));
    //     ImPlot::EndPlot();
    // }
}
void ShowChipButtonComboBox(int &currentItem)
{
    if (ImGui::BeginCombo("", chipButtons[currentItem].name))
    {
        for (int i = 0; i < chipButtons.size(); i++)
        {
            bool isSelected = (currentItem == i);
            if (ImGui::Selectable(chipButtons[i].name, isSelected))
            {
                currentItem = i;
                ChipCommand(currentItem);
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}
void LoadFonts(AppState &appState) // This is called by runnerParams.callbacks.LoadAdditionalFonts
{
    auto runnerParams = HelloImGui::GetRunnerParams();
    runnerParams->dpiAwareParams.onlyUseFontDpiResponsive = true;

    runnerParams->callbacks.defaultIconFont = HelloImGui::DefaultIconFont::FontAwesome4;
    // First, load the default font (the default font should be loaded first)
    HelloImGui::ImGuiDefaultSettings::LoadDefaultFont_WithFontAwesomeIcons();
    // Then load the other fonts
    appState.TitleFont = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Heavy.otf", 40.f);

    // MY FONTS
    HelloImGui::FontLoadingParams fontLoadingTitleFont;
    fontLoadingTitleFont.useFullGlyphRange = true;
    appState.TitleFont = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Black.otf", 40.f, fontLoadingTitleFont); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingColorFont;
    fontLoadingColorFont.useFullGlyphRange = true;
    appState.ColorFont = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Bold.otf", 20.f, fontLoadingColorFont); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingEmojiFont;
    fontLoadingEmojiFont.useFullGlyphRange = true;
    appState.EmojiFont = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Heavy.otf", 20.f, fontLoadingEmojiFont); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingLargeIconFont;
    fontLoadingLargeIconFont.useFullGlyphRange = true;
    appState.LargeIconFont = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Light.otf", 20.f, fontLoadingLargeIconFont); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingButtonFont;
    fontLoadingButtonFont.useFullGlyphRange = true;
    appState.ButtonFont = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Black.otf", 20.f, fontLoadingButtonFont); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingHeaderFont;
    fontLoadingHeaderFont.useFullGlyphRange = true;
    appState.HeaderFont = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Black.otf", 25.f, fontLoadingHeaderFont); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingStatusFont;
    fontLoadingStatusFont.useFullGlyphRange = true;
    appState.StatusFont = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Regular.otf", 20.f, fontLoadingStatusFont); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingPlotFont;
    fontLoadingPlotFont.useFullGlyphRange = true;
    appState.PlotFont = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoText-MediumItalic.otf", 15.f, fontLoadingPlotFont); // will be loaded from the assets folder

    // SAN FRANCISCO FONTS
    // DISPLAY FONTS
    HelloImGui::FontLoadingParams fontLoadingSFDBlack;
    fontLoadingSFDBlack.useFullGlyphRange = true;
    appState.SFDBlack = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Black.otf", 40.f, fontLoadingSFDBlack); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFDBold;
    fontLoadingSFDBold.useFullGlyphRange = true;
    appState.SFDBold = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Bold.otf", 20.f, fontLoadingSFDBold); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFDHeavy;
    fontLoadingSFDHeavy.useFullGlyphRange = true;
    appState.SFDHeavy = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Heavy.otf", 30.f, fontLoadingSFDHeavy); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFDLight;
    fontLoadingSFDLight.useFullGlyphRange = true;
    appState.SFDLight = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Light.otf", 30.f, fontLoadingSFDLight); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFDMedium;
    fontLoadingSFDMedium.useFullGlyphRange = true;
    appState.SFDMedium = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Medium.otf", 30.f, fontLoadingSFDMedium); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFDRegular;
    fontLoadingSFDRegular.useFullGlyphRange = true;
    appState.SFDRegular = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Regular.otf", 30.f, fontLoadingSFDRegular); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFDSemibold;
    fontLoadingSFDSemibold.useFullGlyphRange = true;
    appState.SFDSemibold = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Semibold.otf", 30.f, fontLoadingSFDSemibold); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFDThin;
    fontLoadingSFDThin.useFullGlyphRange = true;
    appState.SFDThin = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Thin.otf", 30.f, fontLoadingSFDThin); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFDUltralight;
    fontLoadingSFDUltralight.useFullGlyphRange = true;
    appState.SFDUltralight = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Ultralight.otf", 30.f, fontLoadingSFDUltralight); // will be loaded from the assets folder

    // TEXT Fonts

    HelloImGui::FontLoadingParams fontLoadingSFTBold;
    fontLoadingSFTBold.useFullGlyphRange = true;
    appState.SFTBold = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoText-Bold.otf", 30.f, fontLoadingSFTBold); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFTBoldItalic;
    fontLoadingSFTBoldItalic.useFullGlyphRange = true;
    appState.SFTBoldItalic = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoText-BoldItalic.otf", 30.f, fontLoadingSFTBoldItalic); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFTHeavy;
    fontLoadingSFTHeavy.useFullGlyphRange = true;
    appState.SFTHeavy = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoText-Heavy.otf", 30.f, fontLoadingSFTHeavy); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFTHeavyItalic;
    fontLoadingSFTHeavyItalic.useFullGlyphRange = true;
    appState.SFTHeavyItalic = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoText-HeavyItalic.otf", 30.f, fontLoadingSFTHeavyItalic); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFTLight;
    fontLoadingSFTLight.useFullGlyphRange = true;
    appState.SFTLight = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoText-Light.otf", 15.f, fontLoadingSFTLight); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFTLightItalic;
    fontLoadingSFTLightItalic.useFullGlyphRange = true;
    appState.SFTLightItalic = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoText-LightItalic.otf", 30.f, fontLoadingSFTLightItalic); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFTMedium;
    fontLoadingSFTMedium.useFullGlyphRange = true;
    appState.SFTMedium = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoText-Medium.otf", 20.f, fontLoadingSFTMedium); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFTMediumItalic;
    fontLoadingSFTMediumItalic.useFullGlyphRange = true;
    appState.SFTMediumItalic = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoText-MediumItalic.otf", 20.f, fontLoadingSFTMediumItalic); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFTRegular;
    fontLoadingSFTRegular.useFullGlyphRange = true;
    appState.SFTRegular = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoText-Regular.otf", 20.f, fontLoadingSFTRegular); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFTRegularItalic;
    fontLoadingSFTRegularItalic.useFullGlyphRange = true;
    appState.SFTRegularItalic = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoText-RegularItalic.otf", 20.f, fontLoadingSFTRegularItalic); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFTRSemibold;
    fontLoadingSFTRSemibold.useFullGlyphRange = true;
    appState.SFTRSemibold = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoText-Semibold.otf", 20.f, fontLoadingSFTRSemibold); // will be loaded from the assets folder

    HelloImGui::FontLoadingParams fontLoadingSFTSemiboldItalic;
    fontLoadingSFTSemiboldItalic.useFullGlyphRange = true;
    appState.SFTSemiboldItalic = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoText-SemiboldItalic.otf", 20.f, fontLoadingSFTSemiboldItalic); // will be loaded from the assets folder
}

const ImVec4 Colors::Red = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
const ImVec4 Colors::Green = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
const ImVec4 Colors::Blue = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
const ImVec4 Colors::Yellow = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
const ImVec4 Colors::Cyan = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
const ImVec4 Colors::Magenta = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
const ImVec4 Colors::White = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
const ImVec4 Colors::Black = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
// ImVec4 redColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Define red color

// Our state
bool show_demo_window = true;
bool show_another_window = false;
bool _flag_send = false;
static int selectedValue = 0;
static int currentChipButtonIndex = 0;
int main(int, char *[])
{
    Colors col_lam;
    AppState appState;
    HelloImGui::RunnerParams runnerParams;
    runnerParams.fpsIdling.enableIdling = false;
    // runnerParams.appWindowParams.windowGeometry.size = {1280, 720};
    runnerParams.appWindowParams.windowTitle = "LAMBDA APP";
    runnerParams.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenWindow; // NoDefaultWindow;
    // params.appWindowParams.restorePreviousGeometry = true;
    // Our application uses a borderless window, but is movable/resizable
    runnerParams.appWindowParams.borderless = true;
    runnerParams.appWindowParams.borderlessMovable = true;
    runnerParams.appWindowParams.borderlessResizable = true;
    runnerParams.appWindowParams.borderlessClosable = true;
    // Load additional font
    runnerParams.callbacks.LoadAdditionalFonts = [&appState]()
    { LoadFonts(appState); };
    auto &tweakedTheme = runnerParams.imGuiWindowParams.tweakedTheme;
    tweakedTheme.Theme = ImGuiTheme::ImGuiTheme_Cherry;
    tweakedTheme.Tweaks.Rounding = 10.f;
    // 2. Customize ImGui style at startup
    runnerParams.callbacks.SetupImGuiStyle = []()
    {
        // Reduce spacing between items ((8, 4) by default)
        ImGui::GetStyle().ItemSpacing = ImVec2(6.f, 4.f);
    };

    runnerParams.callbacks.ShowGui = [&]()
    {
        // HelloImGui::ImageFromAsset("world.jpg"); // Display a static image
        ImGui::PushFont(appState.TitleFont->font);
        ImGui::Indent(20);
        ImGui::Text("LAMBDA");
        ImGui::PopFont();

        ImGui::Separator();
        ImGui::PushFont(appState.HeaderFont->font);
        ImGui::TextColored(Colors::Red, "Lambda voltage UA: %f V", ua);
        ImGui::Spacing();

        ImGui::TextColored(Colors::Yellow, "Heating resistor voltage UR: %f V", ur);
        ImGui::Spacing();

        
        ImGui::TextColored(Colors::Green, "Reference voltage UREF: %f V ", uref);
        ImGui::Spacing();

        ImGui::TextColored(Colors::Blue, "Battery voltage UBAT: %f V", ubat);
        ImGui::Spacing();

        ImGui::TextColored(Colors::Magenta, "Temperature sensor voltage PT1000: %f V", pt1000);
        ImGui::PopFont();
        ImGui::Spacing();

        ImGui::PushFont(appState.StatusFont->font);
        // Use the selected chip button value
        int selectedValue = chipButtons[currentChipButtonIndex].value;
        ShowChipButtonComboBox(currentChipButtonIndex);

        // ImGui::Text("Selected Value: %d", selectedValue);
        ImGui::SameLine();
        if (ImGui::Button("SEND COMMAND"))
        {
            ChipSend(true);
        }

        ImGui::SameLine();

        ImGui::TextColored(col_lam.Red, "ADDRESS %s | RESPONSE %s", chipResponseAddress.c_str(), chipResponseValues.c_str());
        ImGui::PopFont();
        ImGui::Separator();
        ImGui::Spacing();

        if (heatingOn)
        {
            // Button is on, render it in red color
            ImGui::PushStyleColor(ImGuiCol_Button, col_lam.Red);
            ImGui::PushFont(appState.ButtonFont->font);
            if (ImGui::Button("Heating On",ImVec2(120,30)))
            {
                // Toggle the heating state when the button is clicked
                setHeating(false);
                heatingOn = false;
            }
            ImGui::PopFont();
            ImGui::PopStyleColor();
        }
        else
        {
            // Button is off, render it in green color
            ImGui::PushStyleColor(ImGuiCol_Button, col_lam.Green);
            ImGui::PushFont(appState.ButtonFont->font);
            if (ImGui::Button("Heating OFF",ImVec2(120,30)))
            {
                // Toggle the heating state when the button is clicked
                setHeating(true);
                heatingOn = true;
            }
            ImGui::PopFont();
            ImGui::PopStyleColor();
        }
        
        ImGui::SameLine();

        if (ImGui::SliderInt("", &slideVal, 0, 100))
        {
            sliderValue(slideVal);
        }
        ImGui::SameLine();
        ImGui::Text("HEATING SETPOINT");
        // ###### LOGGING
        ImGui::Separator();
        ImGui::Spacing();

        if (loggingOn)
        {
            // Button is on, render it in red color
            ImGui::PushStyleColor(ImGuiCol_Button, col_lam.Green);
            ImGui::PushFont(appState.ButtonFont->font);
            if (ImGui::Button("RECORDING"))
            {
                // Toggle the heating state when the button is clicked
                setLogging(false);
                loggingOn = false;
            }
            ImGui::PopFont();
            ImGui::PopStyleColor();
        }
        else
        {
            // Button is off, render it in green color
            ImGui::PushStyleColor(ImGuiCol_Button, col_lam.Yellow);
            ImGui::PushFont(appState.ButtonFont->font);
            if (ImGui::Button("NOT RECORDING"))
            {
                // Toggle the heating state when the button is clicked
                setLogging(true);
                loggingOn = true;
            }
            ImGui::PopFont();
            ImGui::PopStyleColor();
        }

        // #####################
        // ImGui::PushFont(appState.CustomFont4->font);
        // if (ImGui::SliderInt("Value", &slideVal, 0, 100))
        // {
        //     sliderValue(slideVal);
        // }
        // ImGui::PopFont();

        const auto now = ImGui::GetTime();
        if (now - lastTime >= 0.5)
        {
            lastTime = now;
            // request();
            startFetch("api/val", handleAnalogValues);
        }

        ImGui::Separator();

        ImGui::PushFont(appState.PlotFont->font);
        ImPlot::CreateContext();
        // Demo_LinePlots();
        RealtimePlots();
        ImPlot::DestroyContext();
        ImGui::PopFont();

        ImGui::Separator();

    };
    HelloImGui::Run(runnerParams); //, "Hello, globe", true);

    return 0;
}
// ######################################################################################
