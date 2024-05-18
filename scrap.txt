// #ifndef IMGUI_DEFINE_MATH_OPERATORS
// #define IMGUI_DEFINE_MATH_OPERATORS
// #endif // IMGUI_DEFINE_MATH_OPERATORS
// #ifndef IMPLOT_DISABLE_OBSOLETE_FUNCTIONS
// #define IMPLOT_DISABLE_OBSOLETE_FUNCTIONS
//#endif
#include "hello_imgui/hello_imgui.h"
#include <emscripten/fetch.h>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include "source_implot/implot.h"
#include "source_implot/implot_internal.h"
#include <vector>


// static char szTestText[200];

// float ua, ur,uref,ubat,pt1000;
// void downloadSucceeded(emscripten_fetch_t *fetch) {
//   printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
//   // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
//   emscripten_fetch_close(fetch); // Free data associated with the fetch.
//   memcpy(szTestText, fetch->data, fetch->numBytes);
//   szTestText[fetch->numBytes] = '\0';
//   std::stringstream data(szTestText);


//   const auto read = [&data](float &value) {
//       std::string line;

//       getline(data, line,' ');
//       printf("Read line %s", line.c_str());
//       sscanf(line.c_str(), "%f", &value);
//   };

//   read(ua);
//   read(ur);
//   read(uref); 
//   read(ubat);
//   read(pt1000);

// }

// void downloadFailed(emscripten_fetch_t *fetch) {
//   printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
//   emscripten_fetch_close(fetch); // Also free data on failure.
// }


// void request() {
//     emscripten_fetch_attr_t attr;
//     emscripten_fetch_attr_init(&attr);
//     strcpy(attr.requestMethod, "GET");
//     attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
//     attr.onsuccess = downloadSucceeded;
//     attr.onerror = downloadFailed;
//     emscripten_fetch(&attr, "api/val");
// }
// void sliderValue(int value){
//     emscripten_fetch_attr_t attr;
//     emscripten_fetch_attr_init(&attr);
//     strcpy(attr.requestMethod, "GET");
//     attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
//     // attr.onsuccess = downloadSucceeded;
//     // attr.onerror = downloadFailed;

//     const std::string url = "api/setSlider?v=" + std::to_string(value);
//     emscripten_fetch(&attr, url.c_str());
// }
// int slideVal = 24 ;
// double lastTime= 0.0;


// // utility structure for realtime plot
// struct ScrollingBuffer {
//     int MaxSize;
//     int Offset;
//     ImVector<ImVec2> Data;
//     ScrollingBuffer(int max_size = 2000) {
//         MaxSize = max_size;
//         Offset  = 0;
//         Data.reserve(MaxSize);
//     }
//     void AddPoint(float x, float y) {
//         if (Data.size() < MaxSize)
//             Data.push_back(ImVec2(x,y));
//         else {
//             Data[Offset] = ImVec2(x,y);
//             Offset =  (Offset + 1) % MaxSize;
//         }
//     }
//     void Erase() {
//         if (Data.size() > 0) {
//             Data.shrink(0);
//             Offset  = 0;
//         }
//     }
// };
// // utility structure for realtime plot
// struct RollingBuffer {
//     float Span;
//     ImVector<ImVec2> Data;
//     RollingBuffer() {
//         Span = 10.0f;
//         Data.reserve(2000);
//     }
//     void AddPoint(float x, float y) {
//         float xmod = fmodf(x, Span);
//         if (!Data.empty() && xmod < Data.back().x)
//             Data.shrink(0);
//         Data.push_back(ImVec2(xmod, y));
//     }
// };
// void Demo_LinePlots() {
//     static float xs1[1001], ys1[1001];
    
//     for (int i = 0; i < 1001; ++i) {
//         xs1[i] = i * 0.001f;
//         ys1[i] = 0.5f + 0.5f * sinf(50 * (xs1[i] + (float)ImGui::GetTime() / 10));
//     }
//     static double xs2[20], ys2[20];
//     for (int i = 0; i < 20; ++i) {
//         xs2[i] = i * 1/19.0f;
//         ys2[i] = xs2[i] * xs2[i];
//     }
//     if (ImPlot::BeginPlot("Line Plots")) {
//         ImPlot::SetupAxes("x","y");
//         ImPlot::PlotLine("f(x)", xs1, ys1, 1001);
//         ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
//         ImPlot::PlotLine("g(x)", xs2, ys2, 20,ImPlotLineFlags_Segments);
//         ImPlot::EndPlot();
//     }
// }

// void Demo_RealtimePlots() {
//     ImGui::BulletText("Move your mouse to change the data!");
//     ImGui::BulletText("This example assumes 60 FPS. Higher FPS requires larger buffer size.");
//     static ScrollingBuffer sdata1, sdata2;
//     static RollingBuffer   rdata1, rdata2;
//     ImVec2 mouse = ImGui::GetMousePos();
//     static float t = 0;
//     t += ImGui::GetIO().DeltaTime;
//     sdata1.AddPoint(t, mouse.x * 0.0005f);
//     rdata1.AddPoint(t, mouse.x * 0.0005f);
//     sdata2.AddPoint(t, mouse.y * 0.0005f);
//     rdata2.AddPoint(t, mouse.y * 0.0005f);

//     static float history = 10.0f;
//     ImGui::SliderFloat("History",&history,1,30,"%.1f s");
//     rdata1.Span = history;
//     rdata2.Span = history;

//     static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

//     if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1,150))) {
//         ImPlot::SetupAxes(NULL, NULL, flags, flags);
//         ImPlot::SetupAxisLimits(ImAxis_X1,t - history, t, ImGuiCond_Always);
//         ImPlot::SetupAxisLimits(ImAxis_Y1,0,1);
//         ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL,0.5f);
//         ImPlot::PlotShaded("Mouse X", &sdata1.Data[0].x, &sdata1.Data[0].y, sdata1.Data.size(), -INFINITY, 0, sdata1.Offset, 2 * sizeof(float));
//         ImPlot::PlotLine("Mouse Y", &sdata2.Data[0].x, &sdata2.Data[0].y, sdata2.Data.size(), 0, sdata2.Offset, 2*sizeof(float));
//         ImPlot::EndPlot();
//     }
//     if (ImPlot::BeginPlot("##Rolling", ImVec2(-1,150))) {
//         ImPlot::SetupAxes(NULL, NULL, flags, flags);
//         ImPlot::SetupAxisLimits(ImAxis_X1,0,history, ImGuiCond_Always);
//         ImPlot::SetupAxisLimits(ImAxis_Y1,0,1);
//         ImPlot::PlotLine("Mouse X", &rdata1.Data[0].x, &rdata1.Data[0].y, rdata1.Data.size(), 0, 0, 2 * sizeof(float));
//         ImPlot::PlotLine("Mouse Y", &rdata2.Data[0].x, &rdata2.Data[0].y, rdata2.Data.size(), 0, 0, 2 * sizeof(float));
//         ImPlot::EndPlot();
//     }
// }

// int main(int , char *[]) {   
//     auto guiFunction = []() {
//         ImGui::Text("Hello, ");  
//         ImGui::Text("UA: %f | UR: %f | UREF: %f | UBAT: %f | PT1000: %f",ua,ur,uref,ubat,pt1000);

//         if (ImGui::SliderInt("Value", &slideVal, 0, 100)) {
//             sliderValue(slideVal);
//         }
//                      // Display a simple label
//         HelloImGui::ImageFromAsset("world.jpg");   // Display a static image
//         if(ImGui::Button("Request")) {
//             request();
//         }
//         const auto now = ImGui::GetTime();
//         if (now - lastTime >= 1.0){
//           lastTime = now;
//           request();
//         }
//         ImGui::Text("Values is: %s", szTestText);
//         if (ImGui::Button("Bye!")) {               // Display a button
//             // and immediately handle its action if it is clicked!
//             HelloImGui::GetRunnerParams()->appShallExit = true;
      
//         }
//         // ImGui::CreateContext();
//         ImPlot::CreateContext();
//         Demo_LinePlots();
//         Demo_RealtimePlots();
//         ImPlot::DestroyContext();
//     };
//     HelloImGui::Run(guiFunction, "Hello, globe", true);
    
//     return 0;
    
//######################################################################################



float ua = 0.0f, ur = 0.0f, uref = 0.0f, ubat = 0.0f, pt1000 = 0.0f;
char szTestText[256]; // buffer to hold the fetched data

void downloadSucceeded(emscripten_fetch_t *fetch) {
    printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
    memcpy(szTestText, fetch->data, fetch->numBytes);
    szTestText[fetch->numBytes] = '\0';
    emscripten_fetch_close(fetch); // Free data associated with the fetch.

    std::stringstream data(szTestText);
    const auto read = [&data](float &value) {
        std::string line;
        getline(data, line, ' ');
        sscanf(line.c_str(), "%f", &value);
    };

    read(ua);
    read(ur);
    read(uref); 
    read(ubat);
    read(pt1000);
}


// void Demo_RealtimePlots() {
//     ImGui::BulletText("Realtime Data from FastAPI Server");
//     ImGui::BulletText("This example assumes 60 FPS. Higher FPS requires larger buffer size.");

//     static ScrollingBuffer sdataUa, sdataUr, sdataUref, sdataUbat, sdataPt1000;
//     static RollingBuffer rdataUa, rdataUr, rdataUref, rdataUbat, rdataPt1000;

//     static float t = 0;
//     t += ImGui::GetIO().DeltaTime;

//     sdataUa.AddPoint(t, ua);
//     sdataUr.AddPoint(t, ur);
//     sdataUref.AddPoint(t, uref);
//     sdataUbat.AddPoint(t, ubat);
//     sdataPt1000.AddPoint(t, pt1000);

//     rdataUa.AddPoint(t, ua);
//     rdataUr.AddPoint(t, ur);
//     rdataUref.AddPoint(t, uref);
//     rdataUbat.AddPoint(t, ubat);
//     rdataPt1000.AddPoint(t, pt1000);

//     static float history = 10.0f;
//     ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");
//     rdataUa.Span = history;
//     rdataUr.Span = history;
//     rdataUref.Span = history;
//     rdataUbat.Span = history;
//     rdataPt1000.Span = history;

//     static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

//     if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1, 150))) {
//         ImPlot::SetupAxes(NULL, NULL, flags, flags);
//         ImPlot::SetupAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);
//         ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
//         ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
//         ImPlot::PlotShaded("ua", &sdataUa.Data[0].x, &sdataUa.Data[0].y, sdataUa.Data.size(), -INFINITY, 0, sdataUa.Offset, 2 * sizeof(float));
//         ImPlot::PlotLine("ur", &sdataUr.Data[0].x, &sdataUr.Data[0].y, sdataUr.Data.size(), 0, sdataUr.Offset, 2 * sizeof(float));
//         ImPlot::PlotLine("uref", &sdataUref.Data[0].x, &sdataUref.Data[0].y, sdataUref.Data.size(), 0, sdataUref.Offset, 2 * sizeof(float));
//         ImPlot::PlotLine("ubat", &sdataUbat.Data[0].x, &sdataUbat.Data[0].y, sdataUbat.Data.size(), 0, sdataUbat.Offset, 2 * sizeof(float));
//         ImPlot::PlotLine("pt1000", &sdataPt1000.Data[0].x, &sdataPt1000.Data[0].y, sdataPt1000.Data.size(), 0, sdataPt1000.Offset, 2 * sizeof(float));
//         ImPlot::EndPlot();
//     }
//     if (ImPlot::BeginPlot("##Rolling", ImVec2(-1, 150))) {
//         ImPlot::SetupAxes(NULL, NULL, flags, flags);
//         ImPlot::SetupAxisLimits(ImAxis_X1, 0, history, ImGuiCond_Always);
//         ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
//         ImPlot::PlotLine("ua", &rdataUa.Data[0].x, &rdataUa.Data[0].y, rdataUa.Data.size(), 0, 0, 2 * sizeof(float));
//         ImPlot::PlotLine("ur", &rdataUr.Data[0].x, &rdataUr.Data[0].y, rdataUr.Data.size(), 0, 0, 2 * sizeof(float));
//         ImPlot::PlotLine("uref", &rdataUref.Data[0].x, &rdataUref.Data[0].y, rdataUref.Data.size(), 0, 0, 2 * sizeof(float));
//         ImPlot::PlotLine("ubat", &rdataUbat.Data[0].x, &rdataUbat.Data[0].y, rdataUbat.Data.size(), 0, 0, 2 * sizeof(float));
//         ImPlot::PlotLine("pt1000", &rdataPt1000.Data[0].x, &rdataPt1000.Data[0].y, rdataPt1000.Data.size(), 0, 0, 2 * sizeof(float));
//         ImPlot::EndPlot();
//     }
// }

void downloadFailed(emscripten_fetch_t *fetch) {
    printf("Failed to download %s\n", fetch->url);
    emscripten_fetch_close(fetch); // Free data associated with the fetch.
}

void fetchData() {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;
    emscripten_fetch(&attr, "api/val"); // Replace with your API endpoint
}
// Call fetchData periodically, e.g., using emscripten_set_interval:
EM_JS(void, set_fetch_interval, (), {
    setInterval(_fetchData, 1000); // Fetch data every second
});



int main(int, char *[]) {
    auto guiFunction = []() {
        set_fetch_interval(); // Start the periodic fetching
        ImGui::Text("Hello, world!");

        // Display fetched values
        ImGui::Text("UA: %.2f | UR: %.2f | UREF: %.2f | UBAT: %.2f | PT1000: %.2f", ua, ur, uref, ubat, pt1000);

        if (ImGui::Button("Request Data")) {
            fetchData();
        }

        // Real-time plots
        //Demo_RealtimePlots();

        // Exit button
        if (ImGui::Button("Exit")) {
            HelloImGui::GetRunnerParams()->appShallExit = true;
        }
        //Demo_RealtimePlots();
    };

    HelloImGui::Run(guiFunction, "Real-time Data Visualization", true);
    
    return 0;
}
