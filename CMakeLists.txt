
### CHATGPT ###
cmake_minimum_required(VERSION 3.12)
project(helloworld_with_helloimgui)
set(CMAKE_CXX_STANDARD 17)

# Build hello_imgui
# =================
# 1/ Option 1: if you added hello_imgui as a subfolder, you can add it to your project with:
if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/hello_imgui)
    add_subdirectory(external/hello_imgui)
endif()

# 2/ Option 2: simply fetch hello_imgui during the build
if (NOT TARGET hello_imgui)
    message(STATUS "Fetching hello_imgui")
    include(FetchContent)
    FetchContent_Declare(
        hello_imgui
        GIT_REPOSITORY https://github.com/pthom/hello_imgui.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(hello_imgui)
endif()


# Build your app
# ==============
# List of source files
set(SOURCE_FILES
    main.cpp
    source_implot/implot.cpp
    source_implot/implot_items.cpp
)

hello_imgui_add_app(lambda ${SOURCE_FILES})
# Link the hello_imgui library
target_link_libraries(lambda PRIVATE hello_imgui)

# Link options (if needed)
target_link_options(lambda PRIVATE -sFETCH -sASYNCIFY)
