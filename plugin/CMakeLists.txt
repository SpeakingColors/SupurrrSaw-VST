# Studio 87 SupurrSaw plugin (VST3 + Standalone). The editor is the real mockup page shown in a web view.
cmake_minimum_required(VERSION 3.22)
project(SupurrSaw VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.16-security
    GIT_SHALLOW TRUE)
# offline or local copy: cmake -DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE
FetchContent_MakeAvailable(juce)

# page the plugin shows: ui/supurrsaw-playground.html + bundled fonts + bridge
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(S87_PAGE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../ui/supurrsaw-playground.html)
set(S87_PAGE_OUT ${CMAKE_CURRENT_BINARY_DIR}/web/index.html)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/web)
add_custom_command(OUTPUT ${S87_PAGE_OUT}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/make_plugin_page.py ${S87_PAGE_SRC} ${S87_PAGE_OUT}
    DEPENDS ${S87_PAGE_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/web/shim.js ${CMAKE_CURRENT_SOURCE_DIR}/web/seed_presets.json ${CMAKE_CURRENT_SOURCE_DIR}/tools/make_plugin_page.py
    COMMENT "Building the plugin page from the mockup")
add_custom_target(S87Page DEPENDS ${S87_PAGE_OUT})

juce_add_binary_data(SupurrSawUI SOURCES ${S87_PAGE_OUT})
add_dependencies(SupurrSawUI S87Page)

if(WIN32)
    set(S87_WEBVIEW2 TRUE)
else()
    set(S87_WEBVIEW2 FALSE)
endif()

juce_add_plugin(SupurrSaw
    COMPANY_NAME "Studio 87"
    COMPANY_COPYRIGHT "Studio 87, AGPL-3.0-or-later"
    PLUGIN_MANUFACTURER_CODE Stu8
    PLUGIN_CODE Spsw
    FORMATS VST3 Standalone
    PRODUCT_NAME "SupurrSaw"
    BUNDLE_ID com.studio87.supurrsaw
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Synth
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 ${S87_WEBVIEW2}
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(SupurrSaw PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(SupurrSaw PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)
if(WIN32)
    target_compile_definitions(SupurrSaw PUBLIC JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)
endif()

target_link_libraries(SupurrSaw PRIVATE
    SupurrSawUI
    juce::juce_audio_utils
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

# optional: processor versus core engine check (cmake -DS87_TESTS=ON)
option(S87_TESTS "Build the plugin parity test" OFF)
if(S87_TESTS)
    juce_add_console_app(S87Test PRODUCT_NAME "S87Test" NEEDS_WEB_BROWSER TRUE)
    target_sources(S87Test PRIVATE tests/plugin_test.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(S87Test PRIVATE JUCE_WEB_BROWSER=1 JUCE_USE_CURL=0 "JucePlugin_Name=\"SupurrSaw\"" "JucePlugin_VersionString=\"0.1.0\"")
    target_link_libraries(S87Test PRIVATE SupurrSawUI juce::juce_audio_utils juce::juce_gui_extra juce::juce_recommended_config_flags)

    juce_add_console_app(S87InstTest PRODUCT_NAME "S87InstTest" NEEDS_WEB_BROWSER TRUE)
    target_sources(S87InstTest PRIVATE tests/instrument_test.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(S87InstTest PRIVATE JUCE_WEB_BROWSER=1 JUCE_USE_CURL=0 "JucePlugin_Name=\"SupurrSaw\"" "JucePlugin_VersionString=\"0.1.0\"")
    target_link_libraries(S87InstTest PRIVATE SupurrSawUI juce::juce_audio_utils juce::juce_gui_extra juce::juce_recommended_config_flags)
endif()
