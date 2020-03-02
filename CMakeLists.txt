cmake_minimum_required(VERSION 3.8)
project(loudness_monitor)

if (BUILD_FRUT)
  set(JUCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/JUCE")
  add_subdirectory(3rdparty/FRUT)
else()
  execute_process(COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/FRUT)
  execute_process(COMMAND cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_FRUT=1 -DCMAKE_INSTALL_PREFIX=install ../..
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/FRUT
    COMMAND_ECHO STDOUT)

  execute_process(COMMAND bash -c "make -j`nproc` Jucer2Reprojucer && make install"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/FRUT
    COMMAND_ECHO STDOUT)

  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

  # You have to build and install the above first
  execute_process(COMMAND ${CMAKE_CURRENT_BINARY_DIR}/FRUT/install/FRUT/bin/Jucer2Reprojucer loudmon.jucer ${CMAKE_CURRENT_BINARY_DIR}/FRUT/install/FRUT/cmake/Reprojucer.cmake --relocatable --juce-modules ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/JUCE/modules
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/loudmon
    OUTPUT_VARIABLE Jucer2ReprojucerOutput
    ERROR_VARIABLE Jucer2ReprojucerError
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE)
  message(STATUS "Jucer2Reprojucer: ")
  message(STATUS "  STDOUT: ${Jucer2ReprojucerOutput}")
  message(STATUS "  STDERR: ${Jucer2ReprojucerError}")

  # Custom code starts
  # NOTE: You must use Ninja or Unix Makefiles generator

  set(loudmon_jucer_FILE "${CMAKE_CURRENT_SOURCE_DIR}/loudmon/loudmon.jucer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  add_subdirectory(loudmon)
endif()


