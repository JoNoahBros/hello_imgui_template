# Add emscripten tools to your path
source ~/repos/emsdk/emsdk_env.sh

# cmake and build
mkdir build_emscripten
cd build_emscripten
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release  # or Debug (Release builds lead to smaller files)
make -j 4

# launch a webserver
#python3 -m http.server