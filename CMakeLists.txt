cmake_minimum_required(VERSION 3.8)
project(loudness_monitor)
set(CMAKE_CXX_STANDARD 17)
set(JUCER_VST_LEGACY_SDK_FOLDER ${CMAKE_SOURCE_DIR}/vst2sdk)

if (WIN32)
  set(MSVC_COMPILE_FLAGS "/MP")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MSVC_COMPILE_FLAGS}" )
endif()

if (BUILD_FRUT)
  set(JUCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/JUCE")
  add_subdirectory(3rdparty/FRUT)
else()
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/FRUT)
  execute_process(COMMAND cmake
      -DCMAKE_BUILD_TYPE=Release
      -DBUILD_FRUT=1
      -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/FRUT/install
	  ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/FRUT
    COMMAND_ECHO STDOUT)

  execute_process(COMMAND cmake --build . --target install
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/FRUT
    COMMAND_ECHO STDOUT)

  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

  if (UNIX)
    set(JUCER2REPROJUCER_EXECUTABLE Jucer2Reprojucer)
  elseif(WIN32)
    set(JUCER2REPROJUCER_EXECUTABLE Jucer2Reprojucer.exe)
  endif()
  execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/FRUT/install/FRUT/bin/${JUCER2REPROJUCER_EXECUTABLE} loudmon.jucer ${CMAKE_CURRENT_BINARY_DIR}/FRUT/install/FRUT/cmake/Reprojucer.cmake --relocatable --juce-modules ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/JUCE/modules
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/loudmon
	RESULT_VARIABLE Jucer2ReprojucerStatus
    OUTPUT_VARIABLE Jucer2ReprojucerOutput
    ERROR_VARIABLE Jucer2ReprojucerError
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
	COMMAND_ECHO STDOUT)
  message(STATUS "Jucer2Reprojucer: ")
  message(STATUS "  exit code: ${Jucer2ReprojucerStatus}")
  message(STATUS "  STDOUT: ${Jucer2ReprojucerOutput}")
  message(STATUS "  STDERR: ${Jucer2ReprojucerError}")

  # Custom code starts
  # NOTE: You must use Ninja or Unix Makefiles generator
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  endif()
  set(loudmon_jucer_FILE "${CMAKE_CURRENT_SOURCE_DIR}/loudmon/loudmon.jucer")
  if (PLUGIN_TYPE STREQUAL "Instrument")
    add_compile_definitions(JucePlugin_IsSynth=1)
    add_compile_definitions(JucePlugin_Name="loudmon_Instrument")
    add_compile_definitions(JucePlugin_PluginCode=0x44713031)
    add_compile_definitions(JucePlugin_WantsMidiInput=1)
    add_compile_definitions(JucePlugin_VSTCategory=kPlugCategSynth)
    add_compile_definitions(JucePlugin_Vst3Category="Instrument")
  else()
    add_compile_definitions(JucePlugin_IsSynth=0)
    add_compile_definitions(JucePlugin_Name="loudmon_FX")
    add_compile_definitions(JucePlugin_PluginCode=0x44713032)
    add_compile_definitions(JucePlugin_WantsMidiInput=0)
    add_compile_definitions(JucePlugin_VSTCategory=kPlugCategEffect)
    add_compile_definitions(JucePlugin_Vst3Category="Fx")
  endif()

  add_subdirectory(loudmon)
endif()


