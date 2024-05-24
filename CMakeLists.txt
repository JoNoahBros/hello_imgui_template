# cmake_minimum_required(VERSION 3.12)
# project(helloworld_with_helloimgui)
# set(CMAKE_CXX_STANDARD 17)

# # Build hello_imgui
# # =================
# # 1/  Option 1: if you added hello_imgui as a subfolder, you can add it to your project with:
# if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/hello_imgui)
#     add_subdirectory(external/hello_imgui)
# endif()

# # 2/  Option 2: simply fetch hello_imgui during the build
# if (NOT TARGET hello_imgui)
#     message(STATUS "Fetching hello_imgui")
#     include(FetchContent)
#     FetchContent_Declare(hello_imgui GIT_REPOSITORY https://github.com/pthom/hello_imgui.git GIT_TAG master)
#     FetchContent_MakeAvailable(hello_imgui)
# endif()

# # 3/  Option 3: via vcpkg
# # i/ You can install hello_imgui via vcpkg with:
# #     vcpkg install "hello-imgui[opengl3-binding,glfw-binding]"
# # ii/ Then you can use it inside CMake with:
# #     find_package(hello-imgui CONFIG REQUIRED)


# # Build your app
# # ==============
# hello_imgui_add_app(hello_world_ hello_world.main.cpp source_implot/implot.cpp source_implot/implot_items.cpp)

# target_link_options(hello_world_ PRIVATE -sFETCH)

# # Define the macro globally
# add_compile_definitions(IMGUI_DEFINE_MATH_OPERATORS)

# # Alternatively, define the macro for a specific target
# target_compile_definitions(hello_world_ PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

# target_include_directories(hello_world_ PUBLIC /Users/sabo/repos/emsdk/upstream/emscripten/system/include)

# hello_imgui_add_app is a helper function, similar to cmake's "add_executable"
# Usage:
# hello_imgui_add_app(app_name file1.cpp file2.cpp ...)
#
# Features: 
# * It will automatically link the target to the required libraries (hello_imgui, OpenGl, glad, etc)
# * It will embed the assets (for desktop, mobile, and emscripten apps)
# * It will perform additional customization (app icon and name on mobile platforms, etc)

# Now you can build your app with
#     mkdir build && cd build && cmake .. && cmake --build .

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

# 3/ Option 3: via vcpkg
# i/ You can install hello_imgui via vcpkg with:
#     vcpkg install "hello-imgui[opengl3-binding,glfw-binding]"
# ii/ Then you can use it inside CMake with:
#     find_package(hello-imgui CONFIG REQUIRED)


# Build your app
# ==============
# List of source files
set(SOURCE_FILES
    main.cpp
    source_implot/implot.cpp
    source_implot/implot_items.cpp
)
# hello_imgui_add_app(hello_world_ ${SOURCE_FILES})
# # Link the hello_imgui library
# target_link_libraries(hello_world_ PRIVATE hello_imgui)

# # Link options (if needed)
# target_link_options(hello_world_ PRIVATE -sFETCH -sASYNCIFY)

# # Define the macro for the specific target
# target_compile_definitions(hello_world_ PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

hello_imgui_add_app(lambda ${SOURCE_FILES})
# Link the hello_imgui library
target_link_libraries(lambda PRIVATE hello_imgui)

# Link options (if needed)
target_link_options(lambda PRIVATE -sFETCH -sASYNCIFY)

# Define the macro for the specific target
# target_compile_definitions(lambda PRIVATE IMGUI_DEFINE_MATH_OPERATORS)