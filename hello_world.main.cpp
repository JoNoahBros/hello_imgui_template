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


static char szTestText[200];

float ua, ur,uref,ubat,pt1000;
void downloadSucceeded(emscripten_fetch_t *fetch) {
  printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
  // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
  emscripten_fetch_close(fetch); // Free data associated with the fetch.
  memcpy(szTestText, fetch->data, fetch->numBytes);
  szTestText[fetch->numBytes] = '\0';
  std::stringstream data(szTestText);


  const auto read = [&data](float &value) {
      std::string line;

      getline(data, line,' ');
      printf("Read line %s", line.c_str());
      sscanf(line.c_str(), "%f", &value);
  };

  read(ua);
  read(ur);
  read(uref); 
  read(ubat);
  read(pt1000);

}

void downloadFailed(emscripten_fetch_t *fetch) {
  printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
  emscripten_fetch_close(fetch); // Also free data on failure.
}


void request() {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;
    emscripten_fetch(&attr, "api/val");
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
int slideVal = 24 ;
double lastTime= 0.0;


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

// void Demo_RealtimePlots() {
//     ImGui::BulletText("Move your mouse to change the data!");
   

//     //static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

//     if (ImPlot::BeginPlot("##Scrolling")) {
//         ImPlot::PlotLine("Mouse Y",ua,ubat,100);
//         ImPlot::EndPlot();
//     }
   
// }

int main(int , char *[]) {   
    auto guiFunction = []() {
        ImGui::Text("Hello, ");  
        ImGui::Text("UA: %f | UR: %f | UREF: %f | UBAT: %f | PT1000: %f",ua,ur,uref,ubat,pt1000);

        if (ImGui::SliderInt("Value", &slideVal, 0, 100)) {
            sliderValue(slideVal);
        }
                     // Display a simple label
        HelloImGui::ImageFromAsset("world.jpg");   // Display a static image
        if(ImGui::Button("Request")) {
            request();
        }
        const auto now = ImGui::GetTime();
        if (now - lastTime >= 1.0){
          lastTime = now;
          request();
        }
        ImGui::Text("Values is: %s", szTestText);
        if (ImGui::Button("Bye!")) {               // Display a button
            // and immediately handle its action if it is clicked!
            HelloImGui::GetRunnerParams()->appShallExit = true;
      
        }
        Demo_LinePlots();
        // ImGui::CreateContext();
        // ImPlot::CreateContext();

        
        // Demo_LinePlots();
        // ImPlot::DestroyContext();
        // ImGui::DestroyContext();
    };
    HelloImGui::Run(guiFunction, "Hello, globe", true);
    
    return 0;
    
    //ImGui::End();

}
// // Define a utility structure for real-time plot data
// struct ScrollingBuffer {
//     int MaxSize;
//     int Offset;
//     std::vector<ImVec2> Data;

//     ScrollingBuffer(int max_size = 2000) {
//         MaxSize = max_size;
//         Offset = 0;
//         Data.reserve(MaxSize);
//     }

//     void AddPoint(float x, float y) {
//         if (Data.size() < MaxSize)
//             Data.push_back(ImVec2(x, y));
//         else {
//             Data[Offset] = ImVec2(x, y);
//             Offset = (Offset + 1) % MaxSize;
//         }
//     }
// };

// static char szTestText[200];

// float ua = 0.0f, ur = 0.0f, uref = 0.0f, ubat = 0.0f, pt1000 = 0.0f;

// // Define plotting data buffers
// static ScrollingBuffer sdata_ua, sdata_ur, sdata_uref, sdata_ubat, sdata_pt1000;

// void downloadSucceeded(emscripten_fetch_t *fetch) {
//     printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
//     emscripten_fetch_close(fetch);

//     // Copy received data to szTestText and parse numeric values
//     memcpy(szTestText, fetch->data, fetch->numBytes);
//     szTestText[fetch->numBytes] = '\0';
//     std::stringstream data(szTestText);

//     // Read and assign fetched values
//     data >> ua >> ur >> uref >> ubat >> pt1000;

//     // Add fetched values to plotting buffers
//     sdata_ua.AddPoint(ImGui::GetTime(), ua);
//     sdata_ur.AddPoint(ImGui::GetTime(), ur);
//     sdata_uref.AddPoint(ImGui::GetTime(), uref);
//     sdata_ubat.AddPoint(ImGui::GetTime(), ubat);
//     sdata_pt1000.AddPoint(ImGui::GetTime(), pt1000);
// }

// void downloadFailed(emscripten_fetch_t *fetch) {
//     printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
//     emscripten_fetch_close(fetch);
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

// void Demo_RealtimePlots() {
//     ImGui::BulletText("Real-time Data Plots");

//     // Plot UA over time
//     if (ImPlot::BeginPlot("UA vs. Time", nullptr, nullptr, ImVec2(-1, 150))) {
//         ImPlot::PlotLine("UA", &sdata_ua.Data[0].x, &sdata_ua.Data[0].y, sdata_ua.Data.size(), sdata_ua.Offset, sizeof(ImVec2));
//         ImPlot::EndPlot();
//     }

//     // Plot UR over time
//     if (ImPlot::BeginPlot("UR vs. Time", nullptr, nullptr, ImVec2(-1, 150))) {
//         ImPlot::PlotLine("UR", &sdata_ur.Data[0].x, &sdata_ur.Data[0].y, sdata_ur.Data.size(), sdata_ur.Offset, sizeof(ImVec2));
//         ImPlot::EndPlot();
//     }

//     // Plot UREF over time
//     if (ImPlot::BeginPlot("UREF vs. Time", nullptr, nullptr, ImVec2(-1, 150))) {
//         ImPlot::PlotLine("UREF", &sdata_uref.Data[0].x, &sdata_uref.Data[0].y, sdata_uref.Data.size(), sdata_uref.Offset, sizeof(ImVec2));
//         ImPlot::EndPlot();
//     }

//     // Plot UBAT over time
//     if (ImPlot::BeginPlot("UBAT vs. Time", nullptr, nullptr, ImVec2(-1, 150))) {
//         ImPlot::PlotLine("UBAT", &sdata_ubat.Data[0].x, &sdata_ubat.Data[0].y, sdata_ubat.Data.size(), sdata_ubat.Offset, sizeof(ImVec2));
//         ImPlot::EndPlot();
//     }

//     // Plot PT1000 over time
//     if (ImPlot::BeginPlot("PT1000 vs. Time", nullptr, nullptr, ImVec2(-1, 150))) {
//         ImPlot::PlotLine("PT1000", &sdata_pt1000.Data[0].x, &sdata_pt1000.Data[0].y, sdata_pt1000.Data.size(), sdata_pt1000.Offset, sizeof(ImVec2));
//         ImPlot::EndPlot();
//     }
// }

// int main(int, char *[]) {
//     auto guiFunction = []() {
//         ImGui::Text("Hello, world!");

//         // Display fetched values
//         ImGui::Text("UA: %.2f | UR: %.2f | UREF: %.2f | UBAT: %.2f | PT1000: %.2f", ua, ur, uref, ubat, pt1000);

//         if (ImGui::Button("Request Data")) {
//             request();
//         }

//         // Real-time plots
//         //Demo_RealtimePlots();

//         // Exit button
//         if (ImGui::Button("Exit")) {
//             HelloImGui::GetRunnerParams()->appShallExit = true;
//         }
//     };

//     HelloImGui::Run(guiFunction, "Real-time Data Visualization", true);
    
//     return 0;
// }
