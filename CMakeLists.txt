cmake_minimum_required(VERSION 3.22)
project(LFOSequencer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version" FORCE)

# Add JUCE - adjust this path to where you cloned JUCE
# e.g. ~/JUCE or /usr/local/JUCE
add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/juce EXCLUDE_FROM_ALL)

juce_add_plugin(LFOSequencer
    COMPANY_NAME "LeverStudios"
    COMPANY_COPYRIGHT "Copyright © 2026 Lever Studios. Licensed under the GNU AGPL v3."
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    PLUGIN_MANUFACTURER_CODE Lvrs
    PLUGIN_CODE Lfos
    FORMATS VST3 Standalone
    PRODUCT_NAME "LFO Sequencer"
    DESCRIPTION "4-channel LFO to MIDI sequencer with multiple wave shapes"
)

juce_generate_juce_header(LFOSequencer)

target_sources(LFOSequencer
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/LFOEngine.cpp
        Source/LFOEngine.h
        Source/LFOChannel.cpp
        Source/LFOChannel.h
)

target_compile_definitions(LFOSequencer
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(LFOSequencer
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# macOS-only bundle fixes (xattr, PlistBuddy and plutil don't exist on Windows)
if(APPLE)
    add_custom_command(TARGET LFOSequencer_VST3 POST_BUILD
        # Strip quarantine so Gatekeeper doesn't block the bundle
        COMMAND xattr -cr "$<TARGET_BUNDLE_DIR:LFOSequencer_VST3>"

        # Patch Info.plist — add missing compliance keys
        # (|| true suppresses "key already exists" errors on incremental builds)
        COMMAND /usr/libexec/PlistBuddy
                -c "Add :CFBundleDevelopmentRegion string en"
                -c "Add :CFBundleInfoDictionaryVersion string 6.0"
                -c "Set :NSHumanReadableCopyright Copyright © 2026 Lever Studios. Licensed under the GNU AGPL v3."
                -c "Delete :CFBundleIconFile"
                "$<TARGET_BUNDLE_DIR:LFOSequencer_VST3>/Contents/Info.plist"
                || true

        # plutil -convert xml1 reformats the plist and inserts version="1.0" on the <plist> tag
        COMMAND plutil -convert xml1
                "$<TARGET_BUNDLE_DIR:LFOSequencer_VST3>/Contents/Info.plist"

        COMMENT "Patching Info.plist for VST3 compliance"
        VERBATIM
    )
endif()
