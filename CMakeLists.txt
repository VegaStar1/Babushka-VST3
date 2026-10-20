cmake_minimum_required(VERSION 3.22)
project(Babushka VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "")
endif()

# JUCE: pass -DJUCE_DIR=/path/to/JUCE, otherwise it is fetched (tag 8.0.4).
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Babushka
    COMPANY_NAME "NUWMUN"
    PRODUCT_NAME "Babushka"
    BUNDLE_ID "com.nuwmun.babushka"
    PLUGIN_MANUFACTURER_CODE Nwmn
    PLUGIN_CODE Bbsk
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth Mono
    ICON_BIG Source/ui/art/app_icon_big.png       # Standalone app icon: the NUWMUN logo
    ICON_SMALL Source/ui/art/app_icon_small.png)  # the same, strokes thickened for 16-32 px

# Fonts embedded in the editor (Montserrat and Barlow Semi Condensed, SIL Open Font License 1.1 — see docs/licenses)
# and the NUWMUN logo (vector).
juce_add_binary_data(BabushkaAssets SOURCES
    Source/ui/fonts/Montserrat-Regular.ttf
    Source/ui/fonts/Montserrat-SemiBold.ttf
    Source/ui/fonts/Montserrat-Bold.ttf
    Source/ui/fonts/BarlowSemiCondensed-Medium.ttf
    Source/ui/fonts/BarlowSemiCondensed-SemiBold.ttf
    Source/ui/fonts/BarlowSemiCondensed-Bold.ttf
    Source/ui/art/nuwmun_logo.svg)

target_sources(Babushka PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(Babushka PRIVATE Source)

target_compile_definitions(Babushka PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Babushka
    PRIVATE BabushkaAssets juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Offline renderer / test harness for the DSP core (no JUCE).
add_executable(babushka_render tools/render.cpp)

# Headless DAW-style test of the built VST3 (optional).
option(BABUSHKA_HOST_TEST "Build the VST3 host test" OFF)
if(BABUSHKA_HOST_TEST)
    juce_add_console_app(babushka_hosttest PRODUCT_NAME "babushka_hosttest")
    target_sources(babushka_hosttest PRIVATE tools/hosttest/Main.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(babushka_hosttest PRIVATE Source)
    target_compile_definitions(babushka_hosttest PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(babushka_hosttest PRIVATE BabushkaAssets juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_basics
                          PUBLIC juce::juce_recommended_config_flags)

    # Plays every factory preset and reports level / clipping / silence (optionally writes WAV files).
    juce_add_console_app(babushka_presetaudit PRODUCT_NAME "babushka_presetaudit")
    target_sources(babushka_presetaudit PRIVATE tools/presetaudit/Main.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(babushka_presetaudit PRIVATE Source)
    target_compile_definitions(babushka_presetaudit PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(babushka_presetaudit PRIVATE BabushkaAssets juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_basics juce::juce_dsp
                          PUBLIC juce::juce_recommended_config_flags)

    # Renders the editor (several states) to PNG files; used while working on the look of the interface.
    juce_add_console_app(babushka_uishot PRODUCT_NAME "babushka_uishot")
    target_sources(babushka_uishot PRIVATE tools/uishot/Main.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(babushka_uishot PRIVATE Source)
    target_compile_definitions(babushka_uishot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(babushka_uishot PRIVATE BabushkaAssets juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_basics
                          PUBLIC juce::juce_recommended_config_flags)
endif()
