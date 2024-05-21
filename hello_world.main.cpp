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

static char szTestText[200];
static char responseAPI[200];
bool _flag_start_heating = false;
bool _flag_start_logging = false;
int _cmd_start_logging = 0;
float ua, ur, uref, ubat, pt1000;
int slideVal = 24;
double lastTime = 0.0;
int chip_cmd = 0;
std::string responseText = "Default";

// Define a callback type
using ResponseHandler = std::function<void(const std::string &)>;

struct FetchContext
{
    ResponseHandler handler;
};
void downloadSucceeded(emscripten_fetch_t *fetch)
{
    printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);

    // Retrieve the context
    FetchContext *context = (FetchContext *)fetch->userData;

    // Copy the data to responseAPI
    memcpy(responseAPI, fetch->data, fetch->numBytes);
    responseAPI[fetch->numBytes] = '\0';
    std::string data(responseAPI);

    // Call the appropriate handler
    if (context && context->handler)
    {
        context->handler(data);
    }

    // Clean up
    delete context;
    emscripten_fetch_close(fetch); // Free data associated with the fetch.
}
void downloadFailed(emscripten_fetch_t *fetch)
{
    printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);

    // Clean up
    delete (FetchContext *)fetch->userData;
    emscripten_fetch_close(fetch); // Also free data on failure.
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
    printf("Set logging response: %s\n", data.c_str());
}

void handleSetHeating(const std::string &data)
{
    printf("Set heating response: %s\n", data.c_str());
}
void handleChip(const std::string &data)
{
    printf("Chip response: %s\n", data.c_str());
}

void setLogging(bool value)
{
    // Create a context with the appropriate handler
    FetchContext *context = new FetchContext{handleSetLogging};

    // Initialize fetch attributes
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;
    attr.userData = context;

    // Construct the URL with the logging value
    const std::string url = "api/setLogging?l=" + std::string(value ? "true" : "false");
    printf("Requesting URL: %s\n", url.c_str());
    emscripten_fetch(&attr, url.c_str());
}

void setHeating(bool value)
{
    // Create a context with the appropriate handler
    FetchContext *context = new FetchContext{handleSetHeating};

    // Initialize fetch attributes
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;
    attr.userData = context;

    const std::string url = "api/setHeating?h=" + std::string(value ? "true" : "false");
    printf("Requesting URL: %s\n", url.c_str());
    emscripten_fetch(&attr, url.c_str());
}

void sliderValue(int value)
{
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    // attr.onsuccess = downloadSucceeded;
    // attr.onerror = downloadFailed;

    const std::string url = "api/setSlider?v=" + std::to_string(value);
    emscripten_fetch(&attr, url.c_str());
}
void ChipSend(bool flag)
{
    // Create a context with the appropriate handler
    FetchContext *context = new FetchContext{handleSetHeating};

   // Initialize fetch attributes
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;
    attr.userData = context;
    const std::string url = "api/cmdChip?f=" + std::string(flag ? "true" : "false");
    emscripten_fetch(&attr, url.c_str());
}
void ChipCommand(int value)
{
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    // attr.onsuccess = downloadSucceeded;
    // attr.onerror = downloadFailed;

    const std::string url = "api/cmdChipCommand?v=" + std::to_string(value);
    emscripten_fetch(&attr, url.c_str());
}
void startFetch(const std::string &url, ResponseHandler handler)
{
    FetchContext *context = new FetchContext{handler};

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");

    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed; // Assuming you have a downloadFailed function
    attr.userData = context;

    emscripten_fetch(&attr, url.c_str());
}

// utility structure for realtime plot
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
// utility structure for realtime plot
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

void RealtimePlots()
{
    // ImGui::BulletText("Move your mouse to change the data!");
    // ImGui::BulletText("This example assumes 60 FPS. Higher FPS requires larger buffer size.");
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
    ImGui::SliderFloat("History", &history, 1, 60, "%.1f s");
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

// Demonstrate how to load additional fonts (fonts - part 1/3)
HelloImGui::FontDpiResponsive *gCustomFont = nullptr;
HelloImGui::FontDpiResponsive *gSaboFont = nullptr;
struct AppState
{
    // MyAppSettings myAppSettings; // This values will be stored in the application settings

    HelloImGui::FontDpiResponsive *TitleFont;
    HelloImGui::FontDpiResponsive *ColorFont;
    HelloImGui::FontDpiResponsive *EmojiFont;
    HelloImGui::FontDpiResponsive *LargeIconFont;
    HelloImGui::FontDpiResponsive *CustomFont1;
    HelloImGui::FontDpiResponsive *CustomFont2;
    HelloImGui::FontDpiResponsive *CustomFont3;
    HelloImGui::FontDpiResponsive *CustomFont4;
    HelloImGui::FontDpiResponsive *CustomFont5;
};
struct ChipBtnOld
{
    int IDENT_REG_REQUEST = 0;    //(72, 0)
    int DIAG_REG_REQUEST = 1;     //(120, 0)
    int REG1_INIT_REQUEST = 2;    //(108, 0)
    int REG1_WRITE_REQUEST = 3;   //(86, 0)
    int REG1_CALIBRATE_MODE = 4;  //(86, 157)
    int REG1_NORMAL_MODE_V8 = 5;  //(86, 136)
    int REG1_MODE_NORMAL_V17 = 6; //(86, 137)
    int REG2_INIT_REQUEST = 7;    //(126, 0)
    int REG2_WRITE_REQUEST = 8;   //(90, 0)
};

struct ChipButton {
    const char* name;
    int value;
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
// Example function to display the combo box
void ShowChipButtonComboBox(int& currentItem) {
    if (ImGui::BeginCombo("Chip Buttons", chipButtons[currentItem].name)) {
        for (int i = 0; i < chipButtons.size(); i++) {
            bool isSelected = (currentItem == i);
            if (ImGui::Selectable(chipButtons[i].name, isSelected)) {
                currentItem = i;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}
void MyLoadFonts()
{
    HelloImGui::GetRunnerParams()->dpiAwareParams.onlyUseFontDpiResponsive = true;
    HelloImGui::ImGuiDefaultSettings::LoadDefaultFont_WithFontAwesomeIcons();           // The font that is loaded first is the default font
    gCustomFont = HelloImGui::LoadFontDpiResponsive("fonts/Akronim-Regular.ttf", 40.f); // will be loaded from the assets folder
    gSaboFont = HelloImGui::LoadFontDpiResponsive("fonts/DroidSans.ttf", 20.f);
}
//////////////////////////////////////////////////////////////////////////
//    Additional fonts handling
//////////////////////////////////////////////////////////////////////////
void LoadFonts(AppState &appState) // This is called by runnerParams.callbacks.LoadAdditionalFonts
{
    auto runnerParams = HelloImGui::GetRunnerParams();
    runnerParams->dpiAwareParams.onlyUseFontDpiResponsive = true;

    runnerParams->callbacks.defaultIconFont = HelloImGui::DefaultIconFont::FontAwesome4;
    // First, load the default font (the default font should be loaded first)
    HelloImGui::ImGuiDefaultSettings::LoadDefaultFont_WithFontAwesomeIcons();
    // Then load the other fonts
    appState.TitleFont = HelloImGui::LoadFontDpiResponsive("fonts/Akronim-Regular.ttf", 18.f);

    HelloImGui::FontLoadingParams fontLoadingParamsEmoji;
    fontLoadingParamsEmoji.useFullGlyphRange = true;
    appState.EmojiFont = HelloImGui::LoadFontDpiResponsive("fonts/NotoEmoji-Regular.ttf", 24.f, fontLoadingParamsEmoji);

    HelloImGui::FontLoadingParams fontLoadingParamsLargeIcon;
    fontLoadingParamsLargeIcon.useFullGlyphRange = true;
    appState.LargeIconFont = HelloImGui::LoadFontDpiResponsive("fonts/fontawesome-webfont.ttf", 24.f, fontLoadingParamsLargeIcon);
#ifdef IMGUI_ENABLE_FREETYPE
    // Found at https://www.colorfonts.wtf/
    HelloImGui::FontLoadingParams fontLoadingParamsColor;
    fontLoadingParamsColor.loadColor = true;
    appState.ColorFont = HelloImGui::LoadFontDpiResponsive("fonts/Playbox/Playbox-FREE.otf", 24.f, fontLoadingParamsColor);
#endif
    HelloImGui::FontLoadingParams fontLoadingParamsCustom1;
    fontLoadingParamsCustom1.useFullGlyphRange = true;
    appState.CustomFont1 = HelloImGui::LoadFontDpiResponsive("fonts/Akronim-Regular.ttf", 40.f, fontLoadingParamsCustom1); // will be loaded from the assets folder
    HelloImGui::FontLoadingParams fontLoadingParamsCustom2;
    fontLoadingParamsCustom2.useFullGlyphRange = true;
    appState.CustomFont2 = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Heavy.otf", 20.f, fontLoadingParamsCustom2);
    HelloImGui::FontLoadingParams fontLoadingParamsCustom3;
    fontLoadingParamsCustom3.useFullGlyphRange = true;
    appState.CustomFont3 = HelloImGui::LoadFontDpiResponsive("fonts/SanFrancisco/SanFranciscoDisplay-Black.otf", 25.f, fontLoadingParamsCustom3);
    HelloImGui::FontLoadingParams fontLoadingParamsCustom4;
    fontLoadingParamsCustom4.useFullGlyphRange = true;
    appState.CustomFont4 = HelloImGui::LoadFontDpiResponsive("fonts/AtPinkoRegular-8OO1M.ttf", 18.f, fontLoadingParamsCustom4);
    HelloImGui::FontLoadingParams fontLoadingParamsCustom5;
    fontLoadingParamsCustom5.useFullGlyphRange = true;
    appState.CustomFont5 = HelloImGui::LoadFontDpiResponsive("fonts/AtPinkoRegular-8OO1M.ttf", 15.f, fontLoadingParamsCustom5);
}
// Our state
bool show_demo_window = true;
bool show_another_window = false;
bool _flag_send = false;
static int selectedValue = 0;
static int currentChipButtonIndex = 0;
int main(int, char *[])
{


    
    // LoadFonts();
    //  Our application state
    AppState appState;
    HelloImGui::RunnerParams runnerParams;

    // runnerParams.appWindowParams.windowGeometry.size = {1280, 720};
    runnerParams.appWindowParams.windowTitle = "LAMBDA APP";
    runnerParams.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenWindow; // NoDefaultWindow;

    // params.appWindowParams.restorePreviousGeometry = true;

    // Our application uses a borderless window, but is movable/resizable
    runnerParams.appWindowParams.borderless = true;
    runnerParams.appWindowParams.borderlessMovable = true;
    runnerParams.appWindowParams.borderlessResizable = true;
    runnerParams.appWindowParams.borderlessClosable = true;

    // Fonts need to be loaded at the appropriate moment during initialization (fonts - part 2/3)
    // runnerParams.callbacks.LoadAdditionalFonts = MyLoadFonts; // LoadAdditionalFonts is a callback that we set with our own font loading function
    // Load additional font
    runnerParams.callbacks.LoadAdditionalFonts = [&appState]()
    { LoadFonts(appState); };
    // Demo custom font usage (fonts - part 3/3)
    // auto guiFunction = []() {
    // 1. Change theme
    auto &tweakedTheme = runnerParams.imGuiWindowParams.tweakedTheme;
    tweakedTheme.Theme = ImGuiTheme::ImGuiTheme_MaterialFlat;
    tweakedTheme.Tweaks.Rounding = 10.f;
    // 2. Customize ImGui style at startup
    runnerParams.callbacks.SetupImGuiStyle = []()
    {
        // Reduce spacing between items ((8, 4) by default)
        ImGui::GetStyle().ItemSpacing = ImVec2(6.f, 4.f);
    };

    
    runnerParams.callbacks.ShowGui = [&]()
    {
        // Our application state
        // ImGui::PushFont(gCustomFont->font);
        // ImGui::Text("LAMBDA");
        // ImGui::PopFont();
        // ImGui::PushFont(gSaboFont->font);
        // ImGui::Text("Hello, ");
        // ImGui::Text("UA: %f | UR: %f | UREF: %f | UBAT: %f | PT1000: %f",ua,ur,uref,ubat,pt1000);

        HelloImGui::ImageFromAsset("world.jpg"); // Display a static image
        ImGui::PushFont(appState.CustomFont1->font);
        ImGui::SameLine();
        ImGui::Text("LAMBDA");
        ImGui::PopFont();
        ImGui::PushFont(appState.CustomFont2->font);
        ImGui::Text("UA: %f V", ua);
        ImGui::SameLine();
        ImGui::Text("UR: %f V", ur);
        ImGui::SameLine();
        ImGui::Text("UREF: %f V ", uref);
        ImGui::SameLine();
        ImGui::Text("UBAT: %f V", ubat);
        ImGui::SameLine();
        ImGui::Text("PT1000: %f V", pt1000);
        ImGui::PopFont();

        ImGui::PushFont(appState.CustomFont4->font);
       
        // ImGui::SameLine();

        // if (ImGui::Button("DIAG_REG_REQUEST")){
        //     Chip(chipButtons.DIAG_REG_REQUEST);
        // }
        // ImGui::SameLine();

        // if (ImGui::Button("REG1_INIT_REQUEST")){
        //     Chip(chipButtons.REG1_INIT_REQUEST);
        // }
        // ImGui::SameLine();

        // if (ImGui::Button("REG1_WRITE_REQUEST ID")){
        //     Chip(chipButtons.REG1_WRITE_REQUEST);
        // }
        // ImGui::SameLine();

        // if (ImGui::Button("REG1_CALIBRATE_MODE")){
        //     Chip(chipButtons.REG1_CALIBRATE_MODE);
        // }
        // ImGui::SameLine();

        // if (ImGui::Button("REG1_NORMAL_MODE_V8 ")){
        //     Chip(chipButtons.REG1_NORMAL_MODE_V8);
        // }
        // ImGui::SameLine();

        // if (ImGui::Button("REG1_MODE_NORMAL_V17")){
        //     Chip(chipButtons.REG1_MODE_NORMAL_V17);
        // }
        // ImGui::SameLine();

        // if (ImGui::Button("Read REG2_INIT_REQUEST")){
        //     Chip(chipButtons.REG2_INIT_REQUEST);
        // }
        // ImGui::SameLine();

        // if (ImGui::Button("REG2_WRITE_REQUEST")){
        //     Chip(chipButtons.IDENT_REG_REQUEST);
        // }
        // ImGui::SameLine();
    

        if (ImGui::SliderInt("Value", &slideVal, 0, 100))
        {
            sliderValue(slideVal);
        }
        if (ImGui::Button("Heating On"))
        {
            setHeating(true);
            startFetch("api/setHeating?h=", handleSetHeating);
        }
        ImGui::SameLine();
        if (ImGui::Button("Heating OFF"))
        {
            setHeating(false);
            startFetch("api/setHeating?h=", handleSetHeating);
        }
        ImGui::SameLine();

        if (ImGui::Button("Logging On"))
        {
            _cmd_start_logging = 1;
            startFetch("api/setLogging?l=", handleSetLogging);
            setLogging(true);
        }
        ImGui::SameLine();
        if (ImGui::Button("Logging OFF"))
        {
            _cmd_start_logging = 0;
            startFetch("api/setLogging?l=", handleSetLogging);
            setLogging(false);
        }
        ImGui::PopFont();
        ImGui::PushFont(appState.CustomFont4->font);
        const auto now = ImGui::GetTime();
        if (now - lastTime >= 1)
        {
            lastTime = now;
            // request();
            startFetch("api/val", handleAnalogValues);
        }

        // if (ImGui::Button("Bye!")) {               // Display a button
        //     // and immediately handle its action if it is clicked!
        //     HelloImGui::GetRunnerParams()->appShallExit = true;
        // }
        ImGui::PopFont();

        ImGui::PushFont(appState.CustomFont5->font);
        ImPlot::CreateContext();
        // Demo_LinePlots();
        RealtimePlots();
        ImPlot::DestroyContext();
        ImGui::PopFont();
        ImGui::PushFont(appState.CustomFont3->font);
        // setLogging(_flag_start_logging);
        ImGui::Text("RESPONSE: %s", responseAPI);
        ImGui::PopFont();

            // Use the selected chip button value
        int selectedValue = chipButtons[currentChipButtonIndex].value;
        ShowChipButtonComboBox(currentChipButtonIndex);


        ImGui::Text("Selected Value: %d", selectedValue);
        ImGui::SameLine();
         if (ImGui::Button("SEND COMMAND")){
            ChipCommand(selectedValue);
            ChipSend(true);

        }
        // ChipSend(false);
        


        // // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        // if (show_demo_window)

        //     ImGui::ShowDemoWindow(&show_demo_window);

        // // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        // {
        //     static float f = 0.0f;
        //     static int counter = 0;

        //     ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        //     // Demo custom font usage (fonts - part 3/3)
        //     ImGui::PushFont(gCustomFont->font);
        //     ImGui::Text("Custom font");
        //     ImGui::PopFont();

        //     ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        //     ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        //     ImGui::Checkbox("Another Window", &show_another_window);

        //     ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        //     ImGui::ColorEdit3("clear color", (float*)&params.imGuiWindowParams.backgroundColor); // Edit 3 floats representing a color

        //     if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        //         counter++;
        //     ImGui::SameLine();
        //     ImGui::Text("counter = %d", counter);

        //     ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        //     #ifndef HELLOIMGUI_MOBILEDEVICE
        //     if (ImGui::Button("Quit"))
        //         params.appShallExit = true;
        //     #endif
        //     ImGui::End();
        // }

        // // 3. Show another simple window.
        // if (show_another_window)
        // {
        //     ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        //     ImGui::Text("Hello from another window!");
        //     if (ImGui::Button("Close Me"))
        //         show_another_window = false;
        //     ImGui::End();
        // }

        // printf("sleeping...\n");
        // emscripten_sleep(100);
    };
    HelloImGui::Run(runnerParams); //, "Hello, globe", true);

    return 0;
}
// ######################################################################################
