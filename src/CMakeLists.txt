add_library(balls SHARED main.cpp ball.cpp ball.h) 

add_library(app-glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(app-glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue/)

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_subdirectory(deps/glm)
add_subdirectory(deps/Hd)

target_link_libraries(balls GLESv3 glm Hd EGL android log app-glue)
