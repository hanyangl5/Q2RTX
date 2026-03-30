if(NOT ANDROID)
    return()
endif()

if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    return()
endif()

get_filename_component(Q2RTX_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(Q2RTX_ANDROID_TEMPLATE_DIR "${Q2RTX_REPO_ROOT}/cmake/android")
set(Q2RTX_ANDROID_DIR "${PROJECT_BINARY_DIR}")

file(TO_CMAKE_PATH "${Q2RTX_REPO_ROOT}" Q2RTX_SOURCE_DIR_FOR_ANDROID)
set(Q2RTX_SDL_ANDROID_JAVA_DIR "${Q2RTX_SOURCE_DIR_FOR_ANDROID}/extern/SDL2/android-project/app/src/main/java")

function(q2rtx_stage_android_file relative_path)
    get_filename_component(q2rtx_stage_dir "${Q2RTX_ANDROID_DIR}/${relative_path}" DIRECTORY)
    file(MAKE_DIRECTORY "${q2rtx_stage_dir}")
    configure_file(
        "${Q2RTX_ANDROID_TEMPLATE_DIR}/${relative_path}"
        "${Q2RTX_ANDROID_DIR}/${relative_path}"
        COPYONLY)
endfunction()

foreach(q2rtx_android_file IN ITEMS
    build.gradle
    gradle.properties
    gradlew
    gradlew.bat
    README.md
    settings.gradle
    app/proguard-rules.pro)
    q2rtx_stage_android_file("${q2rtx_android_file}")
endforeach()

file(MAKE_DIRECTORY "${Q2RTX_ANDROID_DIR}/gradle")
file(COPY "${Q2RTX_ANDROID_TEMPLATE_DIR}/gradle/wrapper" DESTINATION "${Q2RTX_ANDROID_DIR}/gradle")

file(MAKE_DIRECTORY "${Q2RTX_ANDROID_DIR}/app")
file(COPY "${Q2RTX_ANDROID_TEMPLATE_DIR}/app/src" DESTINATION "${Q2RTX_ANDROID_DIR}/app")
file(MAKE_DIRECTORY "${Q2RTX_ANDROID_DIR}/app/jni")

configure_file(
    "${Q2RTX_ANDROID_TEMPLATE_DIR}/app-build.gradle.in"
    "${Q2RTX_ANDROID_DIR}/app/build.gradle"
    @ONLY)

configure_file(
    "${Q2RTX_ANDROID_TEMPLATE_DIR}/app-jni-CMakeLists.txt.in"
    "${Q2RTX_ANDROID_DIR}/app/jni/CMakeLists.txt"
    @ONLY)

message(STATUS "Staged Android Gradle project in ${Q2RTX_ANDROID_DIR}")
