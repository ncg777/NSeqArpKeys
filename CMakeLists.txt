cmake_minimum_required(VERSION 3.22)
project(NSeqArpKeys VERSION 1.4.1 LANGUAGES C CXX)
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

juce_add_binary_data(FactoryPatternsData
    NAMESPACE FactoryPatternsData
    HEADER_NAME FactoryPatternsData.h
    SOURCES banks/FactoryPatterns/FactoryPatterns.zip)

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
    FactoryPatternsData
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
        NSeqArpKeys FactoryPatternsData juce::juce_audio_utils juce::juce_recommended_config_flags)
    add_test(NAME PatternDomain COMMAND NSeqArpKeysDomainTests)
    juce_add_console_app(NSeqArpKeysStateTests PRODUCT_NAME "NSeqArpKeys State Tests")
    juce_generate_juce_header(NSeqArpKeysStateTests)
    target_sources(NSeqArpKeysStateTests PRIVATE tests/ProcessorStateTests.cpp)
    target_link_libraries(NSeqArpKeysStateTests PRIVATE
        NSeqArpKeys juce::juce_audio_utils juce::juce_recommended_config_flags)
    add_test(NAME ProcessorState COMMAND NSeqArpKeysStateTests)
    # Exercise actual editor controls, mode changes, bounds, and scrolling.
    # Linux needs a virtual display; Windows/macOS can render off-screen directly.
    if (UNIX AND NOT APPLE)
        find_program(XVFB_RUN xvfb-run)
    endif()
    if (WIN32 OR APPLE OR XVFB_RUN)
        set(editor_test_command $<TARGET_FILE:NSeqArpKeysStateTests>)
        if (UNIX AND NOT APPLE)
            list(PREPEND editor_test_command "${XVFB_RUN}" -a)
        endif()
        add_test(NAME EditorLayout COMMAND ${editor_test_command}
            "${CMAKE_CURRENT_BINARY_DIR}/editor-rhythmic.png"
            "${CMAKE_CURRENT_BINARY_DIR}/editor-bank.png"
            "${CMAKE_CURRENT_BINARY_DIR}/editor-melodic.png"
            "${CMAKE_CURRENT_BINARY_DIR}/editor-small.png"
            "${CMAKE_CURRENT_BINARY_DIR}/editor-presets.png")
        set_tests_properties(EditorLayout PROPERTIES ENVIRONMENT
            "NSEQARPKEYS_PRESET_DIR=${CMAKE_CURRENT_BINARY_DIR}/editor-test-data/Presets")
    endif()
endif()
