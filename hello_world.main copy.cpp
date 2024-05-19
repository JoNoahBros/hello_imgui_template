#include "hello_imgui/hello_imgui.h"
#include <emscripten/fetch.h>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include "source_implot/implot.h"
#include "source_implot/implot_internal.h"
#include <vector>
#include <stdio.h>




static char szTestText[200];
static char responseAPI[200];
bool  _flag_start_heating = false;
bool  _flag_start_logging = false;
int  _cmd_start_logging = 0;
float ua, ur,uref,ubat,pt1000;
int slideVal = 24 ;
double lastTime= 0.0;
double lastTimeX= 0.0;

// Define a callback type
using ResponseHandler = std::function<void(const std::string&)>;

struct FetchContext {
    ResponseHandler handler;
};
void downloadSucceeded(emscripten_fetch_t *fetch) {
    printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);

    // Retrieve the context
    FetchContext* context = (FetchContext*)fetch->userData;

    // Copy the data to responseAPI
    memcpy(responseAPI, fetch->data, fetch->numBytes);
    responseAPI[fetch->numBytes] = '\0';
    std::string data(responseAPI);

    // Call the appropriate handler
    if (context && context->handler) {
        context->handler(data);
    }

    // Clean up
    delete context;
    emscripten_fetch_close(fetch); // Free data associated with the fetch.
}
void downloadFailed(emscripten_fetch_t *fetch) {
    printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);

    // Clean up
    delete (FetchContext*)fetch->userData;
    emscripten_fetch_close(fetch); // Also free data on failure.
}

void handleAnalogValues(const std::string& data) {
    std::stringstream dataStream(data);
    const auto read = [&dataStream](float &value) {
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

void handleSetLogging(const std::string& data) {
    printf("Set logging response: %s\n", data.c_str());
}

void handleSetHeating(const std::string& data) {
    printf("Set heating response: %s\n", data.c_str());
}

void setLogging(bool value) {
    // Create a context with the appropriate handler
    FetchContext* context = new FetchContext{handleSetLogging};

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

void setHeating(bool value) {
    // Create a context with the appropriate handler
    FetchContext* context = new FetchContext{handleSetHeating};

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

void sliderValue(int value){
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    // attr.onsuccess = downloadSucceeded;
    // attr.onerror = downloadFailed;

    const std::string url = "api/setSlider?v=" + std::to_string(value);
    emscripten_fetch(&attr, url.c_str());
}

void startFetch(const std::string& url, ResponseHandler handler) {
    FetchContext* context = new FetchContext{handler};

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
struct ScrollingBuffer {
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer(int max_size = 2000) {
        MaxSize = max_size;
        Offset  = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y) {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x,y));
        else {
            Data[Offset] = ImVec2(x,y);
            Offset =  (Offset + 1) % MaxSize;
        }
    }
    void Erase() {
        if (Data.size() > 0) {
            Data.shrink(0);
            Offset  = 0;
        }
    }
};
// utility structure for realtime plot
struct RollingBuffer {
    float Span;
    ImVector<ImVec2> Data;
    RollingBuffer() {
        Span = 10.0f;
        Data.reserve(2000);
    }
    void AddPoint(float x, float y) {
        float xmod = fmodf(x, Span);
        if (!Data.empty() && xmod < Data.back().x)
            Data.shrink(0);
        Data.push_back(ImVec2(xmod, y));
    }
};
void Demo_LinePlots() {
    static float xs1[1001], ys1[1001];
    
    for (int i = 0; i < 1001; ++i) {
        xs1[i] = i * 0.001f;
        ys1[i] = 0.5f + 0.5f * sinf(50 * (xs1[i] + (float)ImGui::GetTime() / 10));
    }
    static double xs2[20], ys2[20];
    for (int i = 0; i < 20; ++i) {
        xs2[i] = i * 1/19.0f;
        ys2[i] = xs2[i] * xs2[i];
    }
    if (ImPlot::BeginPlot("Line Plots")) {
        ImPlot::SetupAxes("x","y");
        ImPlot::PlotLine("f(x)", xs1, ys1, 1001);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
        ImPlot::PlotLine("g(x)", xs2, ys2, 20,ImPlotLineFlags_Segments);
        ImPlot::EndPlot();
    }
}

void Demo_RealtimePlots() {
    ImGui::BulletText("Move your mouse to change the data!");
    ImGui::BulletText("This example assumes 60 FPS. Higher FPS requires larger buffer size.");
    static ScrollingBuffer sdata1, sdata2,sdata3,sdata4,sdata5,sdata6;
    static RollingBuffer   rdata1, rdata2,rdata3,rdata4,rdata5,rdata6;
    //ImVec2 mouse = ImGui::GetMousePos();
    const auto t = ImGui::GetTime();
    
    sdata1.AddPoint(t, ua);
    sdata2.AddPoint(t, ur);
    sdata3.AddPoint(t, uref);
    sdata4.AddPoint(t, ubat);
    sdata5.AddPoint(t, pt1000);

    static float history = 10.0f;
    ImGui::SliderFloat("History",&history,1,30,"%.1f s");
    rdata1.Span = history;
    rdata2.Span = history;
    rdata3.Span = history;
    rdata4.Span = history;
    rdata5.Span = history;

    static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

    if (ImPlot::BeginPlot("Scrolling",ImVec2(-1,150))) {
        ImPlot::SetupAxes(NULL, NULL, flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1,t - history, t, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1,0,6);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL,0.5f);
        //ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
        ImPlot::PlotLine("UA", &sdata1.Data[0].x, &sdata1.Data[0].y, sdata1.Data.size(), 0, sdata1.Offset, 2 * sizeof(float));
        ImPlot::PlotLine("UR", &sdata2.Data[0].x, &sdata2.Data[0].y, sdata2.Data.size(), 0, sdata2.Offset, 2*sizeof(float));
        ImPlot::PlotLine("UREF", &sdata3.Data[0].x, &sdata3.Data[0].y, sdata3.Data.size(), 0, sdata3.Offset, 2*sizeof(float));
        ImPlot::PlotLine("UBAT", &sdata4.Data[0].x, &sdata4.Data[0].y, sdata4.Data.size(), 0, sdata4.Offset, 2*sizeof(float));
        ImPlot::PlotLine("PT1000", &sdata5.Data[0].x, &sdata5.Data[0].y, sdata5.Data.size(), 0, sdata5.Offset, 2*sizeof(float));
        ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("##Rolling", ImVec2(-1,150))) {
        ImPlot::SetupAxes(NULL, NULL, flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1,0,history, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1,0,1);
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


//////////////////////////////////////////////////////////////////////////
//    Our Application State
//////////////////////////////////////////////////////////////////////////
struct MyAppSettings
{
    HelloImGui::InputTextData motto = HelloImGui::InputTextData(
        "Hello, Dear ImGui\n"
        "Unleash your creativity!\n",
        true, // multiline
        ImVec2(14.f, 3.f) // initial size (in em)
    );
    int value = 10;
};

struct AppState
{
    float f = 0.0f;
    int counter = 0;

    float rocket_launch_time = 0.f;
    float rocket_progress = 0.0f;

    enum class RocketState {
        Init,
        Preparing,
        Launched
    };
    RocketState rocket_state = RocketState::Init;

    MyAppSettings myAppSettings; // This values will be stored in the application settings

	HelloImGui::FontDpiResponsive *TitleFont;
	HelloImGui::FontDpiResponsive *ColorFont;
	HelloImGui::FontDpiResponsive *EmojiFont;
	HelloImGui::FontDpiResponsive *LargeIconFont;
};
//////////////////////////////////////////////////////////////////////////
//    Additional fonts handling
//////////////////////////////////////////////////////////////////////////
void LoadFonts(AppState& appState) // This is called by runnerParams.callbacks.LoadAdditionalFonts
{
	auto runnerParams = HelloImGui::GetRunnerParams();
	runnerParams->dpiAwareParams.onlyUseFontDpiResponsive=true;

    runnerParams->callbacks.defaultIconFont = HelloImGui::DefaultIconFont::FontAwesome6;
    // First, load the default font (the default font should be loaded first)
    HelloImGui::ImGuiDefaultSettings::LoadDefaultFont_WithFontAwesomeIcons();
    // Then load the other fonts
    appState.TitleFont = HelloImGui::LoadFontDpiResponsive("fonts/DroidSans.ttf", 18.f);

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
}
//////////////////////////////////////////////////////////////////////////
//    Save additional settings in the ini file
//////////////////////////////////////////////////////////////////////////
// This demonstrates how to store additional info in the application settings
// Use this sparingly!
// This is provided as a convenience only, and it is not intended to store large quantities of text data.

// Warning, the save/load function below are quite simplistic!
std::string MyAppSettingsToString(const MyAppSettings& myAppSettings)
{
    using namespace nlohmann;
    json j;
    j["motto"] = HelloImGui::InputTextDataToString(myAppSettings.motto);
    j["value"] = myAppSettings.value;
    return j.dump();
}
MyAppSettings StringToMyAppSettings(const std::string& s)
{
    if (s.empty())
        return MyAppSettings();
    MyAppSettings myAppSettings;
    using namespace nlohmann;
    try {
        json j = json::parse(s);
        myAppSettings.motto = HelloImGui::InputTextDataFromString(j["motto"].get<std::string>());
        myAppSettings.value = j["value"];
    }
    catch (json::exception& e)
    {
        HelloImGui::Log(HelloImGui::LogLevel::Error, "Error while parsing user settings: %s", e.what());
    }
    return myAppSettings;
}

// Note: LoadUserSettings() and SaveUserSettings() will be called in the callbacks `PostInit` and `BeforeExit`:
//     runnerParams.callbacks.PostInit = [&appState]   { LoadMyAppSettings(appState);};
//     runnerParams.callbacks.BeforeExit = [&appState] { SaveMyAppSettings(appState);};
void LoadMyAppSettings(AppState& appState) //
{
    appState.myAppSettings = StringToMyAppSettings(HelloImGui::LoadUserPref("MyAppSettings"));
}
void SaveMyAppSettings(const AppState& appState)
{
    HelloImGui::SaveUserPref("MyAppSettings", MyAppSettingsToString(appState.myAppSettings));
}

// Note: LoadUserSettings() and SaveUserSettings() will be called in the callbacks `PostInit` and `BeforeExit`:
//     runnerParams.callbacks.PostInit = [&appState]   { LoadMyAppSettings(appState);};

void DemoRocket(AppState& appState)
{
    ImGui::PushFont(appState.TitleFont->font); ImGui::Text("Status Bar Demo"); ImGui::PopFont();
    ImGui::BeginGroup();
    if (appState.rocket_state == AppState::RocketState::Init)
    {
        if (ImGui::Button(ICON_FA_ROCKET" Launch rocket"))
        {
            appState.rocket_launch_time = (float)ImGui::GetTime();
            appState.rocket_state = AppState::RocketState::Preparing;
            HelloImGui::Log(HelloImGui::LogLevel::Warning, "Rocket is being prepared");
        }
    }
    else if (appState.rocket_state == AppState::RocketState::Preparing)
    {
        ImGui::Text("Please Wait");
        appState.rocket_progress = (float)(ImGui::GetTime() - appState.rocket_launch_time) / 3.f;
        if (appState.rocket_progress >= 1.0f)
        {
            appState.rocket_state = AppState::RocketState::Launched;
            HelloImGui::Log(HelloImGui::LogLevel::Warning, "Rocket was launched");
        }
    }
    else if (appState.rocket_state == AppState::RocketState::Launched)
    {
        ImGui::Text(ICON_FA_ROCKET " Rocket launched");
        if (ImGui::Button("Reset Rocket"))
        {
            appState.rocket_state = AppState::RocketState::Init;
            appState.rocket_progress = 0.f;
        }
    }
    ImGui::EndGroup();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Look at the status bar after clicking");
}

int main(int , char *[]) {   
    auto guiFunction = []() {
        ImGui::Text("Hello, ");  
        ImGui::Text("UA: %f | UR: %f | UREF: %f | UBAT: %f | PT1000: %f",ua,ur,uref,ubat,pt1000);

        if (ImGui::SliderInt("Value", &slideVal, 0, 100)) {
            sliderValue(slideVal);
        }
        if (ImGui::Button("Heating On")){
            setHeating(true);
            startFetch("api/setHeating?h=", handleSetHeating);
            
        }
         if (ImGui::Button("Heating OFF")){
            setHeating(false);
            startFetch("api/setHeating?h=", handleSetHeating);
        }

        HelloImGui::ImageFromAsset("world.jpg");   // Display a static image

        if (ImGui::Button("Logging On")){
            _cmd_start_logging= 1;
            startFetch("api/setLogging?l=", handleSetLogging);
            setLogging(true);

            }
        if (ImGui::Button("Logging OFF")){
            _cmd_start_logging = 0;
            startFetch("api/setLogging?l=", handleSetLogging);
            setLogging(false);

            }
        const auto now = ImGui::GetTime();
        if (now - lastTime >= 1){
            lastTime = now;
            //request();
            startFetch("api/val", handleAnalogValues);

         
        }
        
       
        //setLogging(_flag_start_logging);
        ImGui::Text("Values is: %s", responseAPI);
        if (ImGui::Button("Bye!")) {               // Display a button
            // and immediately handle its action if it is clicked!
            HelloImGui::GetRunnerParams()->appShallExit = true;
      
        }
  
        ImPlot::CreateContext();   
        Demo_RealtimePlots();
        ImPlot::DestroyContext();
    
 
    
    // printf("sleeping...\n");
    // emscripten_sleep(100);
    };
    HelloImGui::Run(guiFunction, "Hello, globe", true);
    
    return 0;
}  
//######################################################################################

