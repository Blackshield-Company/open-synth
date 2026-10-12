cmake_minimum_required(VERSION 3.22)
project(OpenSynth VERSION 2.1.0)

# MSVC: M_PI lives behind _USE_MATH_DEFINES; windows.h min/max macros
# break std::min/std::max without NOMINMAX.
if(MSVC)
    add_compile_definitions(_USE_MATH_DEFINES NOMINMAX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Locate JUCE ──────────────────────────────────────────────────────────────
# Resolution order:
#   1. -DJUCE_DIR=/path/to/JUCE (cache variable)
#   2. JUCE_DIR environment variable
#   3. libs/JUCE git submodule
#   4. ~/.juce (convenience fallback for local dev)
set(JUCE_DIR "" CACHE PATH "Path to the JUCE source tree")
if(NOT JUCE_DIR)
    if(DEFINED ENV{JUCE_DIR})
        set(JUCE_DIR "$ENV{JUCE_DIR}")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/JUCE/CMakeLists.txt")
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/JUCE")
    elseif(EXISTS "$ENV{HOME}/.juce/CMakeLists.txt")
        set(JUCE_DIR "$ENV{HOME}/.juce")
    else()
        message(FATAL_ERROR
            "JUCE not found. Either:\n"
            "  - pass -DJUCE_DIR=/path/to/JUCE\n"
            "  - set the JUCE_DIR environment variable\n"
            "  - clone JUCE into libs/JUCE (git submodule add https://github.com/juce-framework/JUCE libs/JUCE)")
    endif()
endif()
message(STATUS "Using JUCE from: ${JUCE_DIR}")
add_subdirectory(${JUCE_DIR} JUCE_BUILD)

# Add CLAP JUCE extensions
add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL)

# ── iOS / Android detection ──────────────────────────────────────────────────
set(IS_IOS FALSE)
set(IS_ANDROID FALSE)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(IS_IOS TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(IS_ANDROID TRUE)
endif()

# ── Determine formats ────────────────────────────────────────────────────────
# On iOS/Android we build only Standalone (AUv3 is also possible on iOS via Xcode).
# On desktop we build Standalone + VST3 + CLAP.
if(IS_IOS OR IS_ANDROID)
    set(OPEN_SYNTH_FORMATS Standalone)
elseif(APPLE)
    set(OPEN_SYNTH_FORMATS Standalone VST3 AU)
else()
    set(OPEN_SYNTH_FORMATS Standalone VST3)
endif()

# ── Open Synth Plugin + Standalone ───────────────────────────────────────────
juce_add_plugin(OpenSynth
    COMPANY_NAME "synth"
    PLUGIN_MANUFACTURER_CODE "SynS"
    PLUGIN_CODE "OpSy"
    FORMATS ${OPEN_SYNTH_FORMATS}
    PRODUCT_NAME "Open Synth"
    BUNDLE_ID "com.synthalorian.opensynth"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Open Synth needs microphone access for audio recording."
    # iOS specific
    BACKGROUND_AUDIO_ENABLED TRUE
    STATUS_BAR_HIDDEN TRUE
    REQUIRES_FULL_SCREEN FALSE
    IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    TARGETED_DEVICE_FAMILY "1,2"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon_1024.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon_512.png"
    # Android specific (no-op on other platforms)
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE
)

# Add CLAP target (desktop only)
if(NOT IS_IOS AND NOT IS_ANDROID)
    clap_juce_extensions_plugin(TARGET OpenSynth
        CLAP_ID "audio.synthalorian.opensynth"
        CLAP_FEATURES instrument synthesizer
    )
endif()

# Source files
set(OPENSYNTH_SOURCES
    src/plugin_processor.cpp
    src/plugin_editor.cpp
    src/synth_engine_wrapper.cpp
    src/preset_data.cpp
    src/instrument_realism.cpp
    src/user_preset_manager.cpp
    src/app_state_manager.cpp
)

# DSP engine sources (ported from Flutter FFI)
set(DSP_SOURCES
    dsp/voice.cpp
    dsp/voice_allocator.cpp
    dsp/oscillator.cpp
    dsp/envelope.cpp
    dsp/filter.cpp
    dsp/lfo.cpp
    dsp/arpeggiator.cpp
    dsp/drum_synth.cpp
    dsp/drum_kit_mapping.cpp
    dsp/rhythm_pattern_player.cpp
    dsp/wavetable_oscillator.cpp
    dsp/wavetable_data.cpp
    dsp/wavetable_bank.cpp
    dsp/physical_model.cpp
    dsp/synth_engine.cpp
    dsp/synth_preset.cpp
    dsp/synth_ffi.cpp
    dsp/synth_mixer.cpp
    dsp/synth_part.cpp
    dsp/recorder.cpp
    dsp/midi_file.cpp
    dsp/sample_player.cpp
    dsp/sample_stream.cpp
    dsp/fx_engine.cpp
    dsp/legacy_fx.cpp
    dsp/fx_eq.cpp
    dsp/fx_limiter.cpp
    dsp/fx_rotary.cpp
    dsp/fx_tremolo.cpp
    dsp/fx_autowah.cpp
    dsp/fx_bitcrusher.cpp
    dsp/fx_ringmod.cpp
    dsp/fx_pitchshift.cpp
    dsp/fx_multitap_delay.cpp
    dsp/fx_pingpong_delay.cpp
    dsp/fx_spring_reverb.cpp
    dsp/fx_gated_reverb.cpp
    dsp/fx_amp_sim.cpp
    dsp/fx_stereo_widener.cpp
    dsp/fx_vocoder.cpp
    # Phase 6: Juno-Di FX parity stubs
    dsp/fx_distortion.cpp
    dsp/fx_overdrive.cpp
    dsp/fx_fuzz.cpp
    dsp/fx_tube_drive.cpp
    dsp/fx_resonant_filter.cpp
    dsp/fx_formant_filter.cpp
    dsp/fx_comb_filter.cpp
    dsp/fx_talk_box.cpp
    dsp/fx_vibrato.cpp
    dsp/fx_auto_pan.cpp
    dsp/fx_uni_vibe.cpp
    dsp/fx_chorus_ensemble.cpp
    dsp/fx_dimension_d.cpp
    dsp/fx_reverse_delay.cpp
    dsp/fx_tape_delay.cpp
    dsp/fx_analog_delay.cpp
    dsp/fx_diffusion_delay.cpp
    dsp/fx_room_reverb.cpp
    dsp/fx_hall_reverb.cpp
    dsp/fx_plate_reverb.cpp
    dsp/fx_shimmer_reverb.cpp
    dsp/fx_non_linear_reverb.cpp
    dsp/fx_harmonizer.cpp
    dsp/fx_octaver.cpp
    dsp/fx_detune.cpp
    dsp/fx_noise_gate.cpp
    dsp/fx_de_esser.cpp
    dsp/fx_transient_shaper.cpp
    dsp/fx_multiband_compressor.cpp
    dsp/fx_lo_fi.cpp
    dsp/fx_vinyl_simulator.cpp
    dsp/fx_radio_simulator.cpp
    dsp/fx_telephone_simulator.cpp
    dsp/fx_cabinet_simulator.cpp
    dsp/fx_graphic_eq.cpp
    dsp/fx_parametric_eq.cpp
    dsp/fx_wah_wah.cpp
    dsp/fx_maximizer.cpp
    dsp/fx_exciter.cpp
    dsp/fx_stereo_imager.cpp
    dsp/fx_resonator.cpp
    dsp/fx_grain_delay.cpp
    dsp/fx_spectral_freeze.cpp
)

target_sources(OpenSynth PRIVATE 
    ${OPENSYNTH_SOURCES}
    ${DSP_SOURCES}
)

target_include_directories(OpenSynth PRIVATE 
    src
    dsp
    include
)

# JUCE modules
# juce_audio_devices is required for mobile audio I/O (CoreAudio on iOS, Oboe/AAudio on Android)
target_link_libraries(OpenSynth PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_devices
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_opengl
)

# Compile definitions
# JUCE_USE_ANDROID_OBOE ensures AAudio/Oboe path on Android when available.
target_compile_definitions(OpenSynth PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_ANDROID_OBOE=1
    JUCE_USE_FLAC=1
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(OpenSynth PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wuninitialized
        -Wunused-parameter
        -Wmissing-field-initializers
        -Wnon-virtual-dtor
    )
endif()

# ── Copy sample libraries to build output ────────────────────────────────────
# Each format gets its own samples/ folder: beside the binary on Windows/Linux,
# and in Contents/Resources on macOS bundles (keeps code-signing seals valid).
set(SAMPLE_DIRS
    piano-upright-kw
    piano-splendid-grand
    drums-muldjordkit
    bass-electric-yr
    guitar-spanish-classical
    drums-colombo-adk
    guitar-electric-clean
    manifests
)

set(OPEN_SYNTH_SAMPLE_TARGETS OpenSynth_Standalone OpenSynth_VST3)
if(NOT IS_IOS AND NOT IS_ANDROID)
    list(APPEND OPEN_SYNTH_SAMPLE_TARGETS OpenSynth_CLAP)
endif()
if(TARGET OpenSynth_AU)
    list(APPEND OPEN_SYNTH_SAMPLE_TARGETS OpenSynth_AU)
endif()

foreach(tgt ${OPEN_SYNTH_SAMPLE_TARGETS})
    if(APPLE AND NOT IS_IOS)
        set(sample_dest "$<TARGET_BUNDLE_CONTENT_DIR:${tgt}>/Resources/samples")
    else()
        set(sample_dest "$<TARGET_FILE_DIR:${tgt}>/samples")
    endif()
    foreach(dir ${SAMPLE_DIRS})
        if(EXISTS "${CMAKE_SOURCE_DIR}/samples/${dir}")
            add_custom_command(TARGET ${tgt} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/samples/${dir}"
                "${sample_dest}/${dir}"
            )
        endif()
    endforeach()
endforeach()

# Linux specific
if(UNIX AND NOT APPLE AND NOT IS_ANDROID)
    target_link_libraries(OpenSynth PRIVATE pthread dl)
endif()

# iOS / Android specific linker/framework tweaks
if(IS_IOS)
    # Ensure we link the required iOS frameworks for audio
    target_link_libraries(OpenSynth PRIVATE
        "-framework AudioToolbox"
        "-framework AVFoundation"
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework Accelerate"
    )
elseif(IS_ANDROID)
    # Android NDK already provides OpenSL ES / AAudio via juce_audio_devices,
    # but we explicitly ensure log linking.
    target_link_libraries(OpenSynth PRIVATE log)
endif()

# ── Sample Load Test ─────────────────────────────────────────────────────────
add_executable(sample_load_test
    tests/sample_load_test.cpp
    dsp/sample_player.cpp
    dsp/sample_stream.cpp
)
target_include_directories(sample_load_test PRIVATE src dsp include)
target_link_libraries(sample_load_test PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_core
)
target_compile_definitions(sample_load_test PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_FLAC=1
)

# ── Sample Player Block Test ─────────────────────────────────────────────────
add_executable(sample_player_block_test
    tests/sample_player_block_test.cpp
    dsp/sample_player.cpp
    dsp/sample_stream.cpp
)
target_include_directories(sample_player_block_test PRIVATE src dsp include)
target_link_libraries(sample_player_block_test PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_core
)
target_compile_definitions(sample_player_block_test PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_FLAC=1
)

# ── FLAC Decode Test ─────────────────────────────────────────────────────────
add_executable(flac_test
    tests/flac_test.cpp
)
target_include_directories(flac_test PRIVATE src dsp include)
target_link_libraries(flac_test PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_core
)
target_compile_definitions(flac_test PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_FLAC=1
)

# ── FX Unit Tests ────────────────────────────────────────────────────────────
add_executable(fx_tests
    dsp/fx_tests.cpp
    ${DSP_SOURCES}
    src/instrument_realism.cpp
)
target_include_directories(fx_tests PRIVATE src dsp include)
target_link_libraries(fx_tests PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_core
    juce::juce_dsp
)
target_compile_definitions(fx_tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

# ── Engine Render Test (noise floor repro) ───────────────────────────────────
add_executable(engine_render_test
    tests/engine_render_test.cpp
    ${DSP_SOURCES}
    src/instrument_realism.cpp
    src/preset_data.cpp
    src/synth_engine_wrapper.cpp
)
target_include_directories(engine_render_test PRIVATE src dsp include)
target_link_libraries(engine_render_test PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_dsp
)
target_compile_definitions(engine_render_test PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_FLAC=1
)
add_executable(sample_static_test
    tests/sample_static_test.cpp
    dsp/sample_player.cpp
    dsp/sample_stream.cpp
)
target_include_directories(sample_static_test PRIVATE src dsp include)
target_link_libraries(sample_static_test PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_core
)
target_compile_definitions(sample_static_test PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_FLAC=1
)

# ── Preset Browser UI Tests (headless) ───────────────────────────────────────
add_executable(preset_browser_test
    tests/preset_browser_test.cpp
)
target_include_directories(preset_browser_test PRIVATE src dsp include)
target_link_libraries(preset_browser_test PRIVATE
    OpenSynth
    juce::juce_gui_basics
    juce::juce_audio_processors
)
target_compile_definitions(preset_browser_test PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

# ── Offline demo renderer (headless) ─────────────────────────────────────────
add_executable(demo_render
    tools/demo_render.cpp
    ${DSP_SOURCES}
    src/instrument_realism.cpp
    src/preset_data.cpp
    src/synth_engine_wrapper.cpp
)
target_include_directories(demo_render PRIVATE src dsp include)
target_link_libraries(demo_render PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_dsp
)
target_compile_definitions(demo_render PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_FLAC=1
)
