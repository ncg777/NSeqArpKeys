cmake_minimum_required(VERSION 3.22)
project(NSeqArpKeys VERSION 1.4.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (DEFINED JUCE_SOURCE_DIR)
    add_subdirectory("${JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(plugin_formats VST3)
if (APPLE)
    list(APPEND plugin_formats AU)
endif()

juce_add_plugin(NSeqArpKeys
    COMPANY_NAME "yourcompany"
    BUNDLE_ID "com.yourcompany.NSeqArpKeys"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE E9fj
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/brand/icon-512.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/assets/brand/icon-128.png"
    FORMATS ${plugin_formats}
    PRODUCT_NAME "NSeqArpKeys")

juce_generate_juce_header(NSeqArpKeys)

juce_add_binary_data(FourthAtlasData
    NAMESPACE FourthAtlasData
    HEADER_NAME FourthAtlasData.h
    SOURCES banks/FourthAtlas/FourthAtlas.zip)

target_sources(NSeqArpKeys PRIVATE
    Source/CollectionUtils.cpp
    Source/Numbers.cpp
    Source/Composition.cpp
    Source/Combination.cpp
    Source/Sequence.cpp
    Source/Pcs12.cpp
    Source/ForteCSV.cpp
    Source/Engine/GateRunnerEngine.cpp
    Source/Engine/PatternScheduler.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(NSeqArpKeys PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(NSeqArpKeys PRIVATE
    FourthAtlasData
    juce::juce_audio_utils
    juce::juce_midi_ci
    PUBLIC
    juce::juce_recommended_config_flags)

include(CTest)
if (BUILD_TESTING)
    juce_add_console_app(NSeqArpKeysTests PRODUCT_NAME "NSeqArpKeys Tests")
    juce_generate_juce_header(NSeqArpKeysTests)
    target_sources(NSeqArpKeysTests PRIVATE tests/PatternSchedulerTests.cpp)
    target_link_libraries(NSeqArpKeysTests PRIVATE
        NSeqArpKeys
        juce::juce_audio_utils
        juce::juce_midi_ci
        juce::juce_recommended_config_flags)
    add_test(NAME PatternScheduler COMMAND NSeqArpKeysTests)
    juce_add_console_app(NSeqArpKeysDomainTests PRODUCT_NAME "NSeqArpKeys Domain Tests")
    juce_generate_juce_header(NSeqArpKeysDomainTests)
    target_sources(NSeqArpKeysDomainTests PRIVATE tests/PatternDomainTests.cpp)
    target_link_libraries(NSeqArpKeysDomainTests PRIVATE
        NSeqArpKeys FourthAtlasData juce::juce_audio_utils juce::juce_recommended_config_flags)
    add_test(NAME PatternDomain COMMAND NSeqArpKeysDomainTests)
    juce_add_console_app(NSeqArpKeysStateTests PRODUCT_NAME "NSeqArpKeys State Tests")
    juce_generate_juce_header(NSeqArpKeysStateTests)
    target_sources(NSeqArpKeysStateTests PRIVATE tests/ProcessorStateTests.cpp)
    target_link_libraries(NSeqArpKeysStateTests PRIVATE
        NSeqArpKeys juce::juce_audio_utils juce::juce_recommended_config_flags)
    add_test(NAME ProcessorState COMMAND NSeqArpKeysStateTests)
endif()
