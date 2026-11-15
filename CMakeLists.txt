cmake_minimum_required(VERSION 3.22)
project(VCASSL VERSION 1.2.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(VCASSL_COPY_PLUGIN_AFTER_BUILD "Install VST3 after building (may require elevation)" OFF)
set(VCASSL_JUCE_PATH "C:/JUCE" CACHE PATH "Optional JUCE 7/8/9 source checkout")
if(EXISTS "${VCASSL_JUCE_PATH}/CMakeLists.txt")
    file(STRINGS "${VCASSL_JUCE_PATH}/CMakeLists.txt" _juce_version REGEX "project\\(JUCE VERSION [789]\\.")
endif()
if(_juce_version)
    add_subdirectory("${VCASSL_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE URL https://github.com/juce-framework/JUCE/archive/refs/tags/9.0.5.tar.gz DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()
juce_add_plugin(VCASSL
    COMPANY_NAME "Papielguapo"
    BUNDLE_ID "com.papielguapo.vcassl"
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${VCASSL_COPY_PLUGIN_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE Papi PLUGIN_CODE VcSL
    FORMATS VST3 Standalone PRODUCT_NAME "VCA SSL")
target_sources(VCASSL PRIVATE Source/PluginProcessor.h Source/PluginProcessor.cpp Source/PluginEditor.h Source/PluginEditor.cpp)
target_compile_definitions(VCASSL PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(VCASSL PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
option(VCASSL_BUILD_TESTS "Build DSP verification" ON)
if(VCASSL_BUILD_TESTS)
 enable_testing()
 juce_add_console_app(VCASSLTests PRODUCT_NAME "VCASSLTests")
 target_sources(VCASSLTests PRIVATE Tests/Tests.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
 target_compile_definitions(VCASSLTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
 target_link_libraries(VCASSLTests PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
 add_test(NAME DSP COMMAND VCASSLTests)
endif()
