add_subdirectory(deps/stb)

set(SRC
    src/Hd/Window.cpp
    src/Hd/Shader.cpp
)

add_library(Hd ${SRC})
target_link_libraries(Hd stb app-glue)

target_include_directories(Hd PUBLIC include)
