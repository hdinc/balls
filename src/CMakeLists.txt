add_library(balls SHARED main.cpp ball.cpp ball.h)

add_library( app-glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c )
target_include_directories(app-glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)

add_subdirectory(deps/glm)
add_subdirectory(deps/Hd)

target_link_libraries(balls glm Hd app-glue GLESv3 EGL android log)
