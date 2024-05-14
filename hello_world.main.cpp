#include "hello_imgui/hello_imgui.h"
#include <emscripten/fetch.h>

static char szTestText[200];

void downloadSucceeded(emscripten_fetch_t *fetch) {
  printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
  // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
  emscripten_fetch_close(fetch); // Free data associated with the fetch.
  memcpy(szTestText, fetch->data, fetch->numBytes);
  szTestText[fetch->numBytes] = '\0';
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
    emscripten_fetch(&attr, "http://localhost:8000/val");
}
int slideVal = 24 ;

int main(int , char *[]) {   
    auto guiFunction = []() {
        ImGui::Text("Hello, ");   
        ImGui::SliderInt("Value", &slideVal, 0, 100) ;               // Display a simple label
        HelloImGui::ImageFromAsset("world.jpg");   // Display a static image
        if(ImGui::Button("Request")) {
            request();
        }
        ImGui::Text("Values is: %s", szTestText);
        if (ImGui::Button("Bye!"))                 // Display a button
            // and immediately handle its action if it is clicked!
            HelloImGui::GetRunnerParams()->appShallExit = true;
     };
    HelloImGui::Run(guiFunction, "Hello, globe", true);
    return 0;
}
