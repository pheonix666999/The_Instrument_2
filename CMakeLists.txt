cmake_minimum_required(VERSION 3.22)

project(MelodyForgePro VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MFPR_ENABLE_TESTS "Build MelodyForgePro tests" ON)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/External/JUCE/CMakeLists.txt")
  message(FATAL_ERROR "JUCE submodule not found. Run: git submodule update --init --recursive")
endif()

add_subdirectory(External/JUCE)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(MFPR_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
set(MFPR_ASSET_STAMP "${MFPR_RESOURCES_DIR}/.assets_generated_stamp")
set(MFPR_COMPRESSED_DIR "${CMAKE_CURRENT_BINARY_DIR}/mfpr_compressed_assets")
set(MFPR_COMPRESSED_STAMP "${MFPR_COMPRESSED_DIR}/.assets_compressed_stamp")

file(MAKE_DIRECTORY "${MFPR_RESOURCES_DIR}")

function(mfpr_ensure_assets)
  set(_sentinel_midi "${MFPR_RESOURCES_DIR}/MIDI/ChordProgressions/ChordProg_299.mid")
  set(_sentinel_shot "${MFPR_RESOURCES_DIR}/MIDI/MelodyOneShots/MelodyShot_199.mid")
  set(_sentinel_json "${MFPR_RESOURCES_DIR}/Presets/Preset_199.json")

  if(NOT EXISTS "${_sentinel_midi}" OR NOT EXISTS "${_sentinel_shot}" OR NOT EXISTS "${_sentinel_json}")
    message(STATUS "Generating placeholder embedded assets (expected 700 files)...")
    execute_process(
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_assets.py" "${MFPR_RESOURCES_DIR}"
      RESULT_VARIABLE _gen_result
    )
    if(NOT _gen_result EQUAL 0)
      message(FATAL_ERROR "Asset generation failed with exit code ${_gen_result}.")
    endif()
    file(WRITE "${MFPR_ASSET_STAMP}" "generated\n")
  endif()
endfunction()

mfpr_ensure_assets()

file(MAKE_DIRECTORY "${MFPR_COMPRESSED_DIR}")

function(mfpr_ensure_compressed_assets)
  set(_sentinel_midi "${MFPR_COMPRESSED_DIR}/MIDI/ChordProgressions/ChordProg_299.mid")
  set(_sentinel_shot "${MFPR_COMPRESSED_DIR}/MIDI/MelodyOneShots/MelodyShot_199.mid")
  set(_sentinel_json "${MFPR_COMPRESSED_DIR}/Presets/Preset_199.json")

  if(NOT EXISTS "${_sentinel_midi}" OR NOT EXISTS "${_sentinel_shot}" OR NOT EXISTS "${_sentinel_json}")
    message(STATUS "Compressing assets for BinaryData embedding (gzip)...")
    execute_process(
      COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/compress_assets.py" "${MFPR_RESOURCES_DIR}" "${MFPR_COMPRESSED_DIR}"
      RESULT_VARIABLE _zip_result
    )
    if(NOT _zip_result EQUAL 0)
      message(FATAL_ERROR "Asset compression failed with exit code ${_zip_result}.")
    endif()
    file(WRITE "${MFPR_COMPRESSED_STAMP}" "compressed\n")
  endif()
endfunction()

mfpr_ensure_compressed_assets()

add_custom_command(
  OUTPUT "${MFPR_ASSET_STAMP}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_assets.py" "${MFPR_RESOURCES_DIR}"
  COMMAND "${CMAKE_COMMAND}" -E touch "${MFPR_ASSET_STAMP}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_assets.py"
  COMMENT "Regenerating placeholder embedded assets (700 files)"
  VERBATIM
)

add_custom_target(MFPRGenerateAssets DEPENDS "${MFPR_ASSET_STAMP}")

add_custom_command(
  OUTPUT "${MFPR_COMPRESSED_STAMP}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/compress_assets.py" "${MFPR_RESOURCES_DIR}" "${MFPR_COMPRESSED_DIR}"
  COMMAND "${CMAKE_COMMAND}" -E touch "${MFPR_COMPRESSED_STAMP}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/compress_assets.py" MFPRGenerateAssets
  COMMENT "Compressing assets for BinaryData embedding (gzip)"
  VERBATIM
)

add_custom_target(MFPRCompressAssets DEPENDS "${MFPR_COMPRESSED_STAMP}")
add_dependencies(MFPRCompressAssets MFPRGenerateAssets)

set(MFPR_RESOURCE_FILES)

foreach(i RANGE 0 299)
  set(idx "${i}")
  string(LENGTH "${idx}" idx_len)
  if(idx_len EQUAL 1)
    set(idx "00${idx}")
  elseif(idx_len EQUAL 2)
    set(idx "0${idx}")
  endif()
  list(APPEND MFPR_RESOURCE_FILES "${MFPR_COMPRESSED_DIR}/MIDI/ChordProgressions/ChordProg_${idx}.mid")
endforeach()

foreach(i RANGE 0 199)
  set(idx "${i}")
  string(LENGTH "${idx}" idx_len)
  if(idx_len EQUAL 1)
    set(idx "00${idx}")
  elseif(idx_len EQUAL 2)
    set(idx "0${idx}")
  endif()
  list(APPEND MFPR_RESOURCE_FILES "${MFPR_COMPRESSED_DIR}/MIDI/MelodyOneShots/MelodyShot_${idx}.mid")
endforeach()

foreach(i RANGE 0 199)
  set(idx "${i}")
  string(LENGTH "${idx}" idx_len)
  if(idx_len EQUAL 1)
    set(idx "00${idx}")
  elseif(idx_len EQUAL 2)
    set(idx "0${idx}")
  endif()
  list(APPEND MFPR_RESOURCE_FILES "${MFPR_COMPRESSED_DIR}/Presets/Preset_${idx}.json")
endforeach()

list(LENGTH MFPR_RESOURCE_FILES MFPR_RESOURCE_COUNT)
if(NOT MFPR_RESOURCE_COUNT EQUAL 700)
  message(FATAL_ERROR "Expected exactly 700 embedded assets, found ${MFPR_RESOURCE_COUNT}.")
endif()

juce_add_binary_data(MelodyForgeProBinaryData
  SOURCES ${MFPR_RESOURCE_FILES}
  HEADER_NAME BinaryData.h
)
add_dependencies(MelodyForgeProBinaryData MFPRCompressAssets)

add_library(MelodyForgeProCore STATIC)

target_sources(MelodyForgeProCore PRIVATE
  Source/AssetLibrary.cpp
  Source/AssetLibrary.h
  Source/FxChain.cpp
  Source/FxChain.h
  Source/LookAndFeel.cpp
  Source/LookAndFeel.h
  Source/MelodyGenerator.cpp
  Source/MelodyGenerator.h
  Source/MidiExporter.cpp
  Source/MidiExporter.h
  Source/ParticlesComponent.cpp
  Source/ParticlesComponent.h
  Source/PianoRollComponent.cpp
  Source/PianoRollComponent.h
  Source/PluginEditor.cpp
  Source/PluginEditor.h
  Source/PluginProcessor.cpp
  Source/PluginProcessor.h
  Source/PresetManager.cpp
  Source/PresetManager.h
  Source/SamplerSlotsComponent.cpp
  Source/SamplerSlotsComponent.h
  Source/SynthEngine.cpp
  Source/SynthEngine.h
)

target_link_libraries(MelodyForgeProCore
  PUBLIC
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    MelodyForgeProBinaryData
)

target_compile_definitions(MelodyForgeProCore
  PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(MelodyForgeProCore
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

set(MFPR_PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND MFPR_PLUGIN_FORMATS AU)
endif()

juce_add_plugin(MelodyForgePro
  COMPANY_NAME "xAI Music Tools"
  PRODUCT_NAME "MelodyForgePro"
  BUNDLE_ID "com.xai.musictools.melodyforgepro"

  PLUGIN_MANUFACTURER_CODE xAIM
  PLUGIN_CODE MFPr

  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT TRUE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE

  COPY_PLUGIN_AFTER_BUILD FALSE

  FORMATS ${MFPR_PLUGIN_FORMATS}
)

target_link_libraries(MelodyForgePro PUBLIC MelodyForgeProCore)

target_compile_definitions(MelodyForgePro
  PRIVATE
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)

enable_testing()
if(MFPR_ENABLE_TESTS)
  add_subdirectory(tests)
endif()
