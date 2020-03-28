cmake_minimum_required(VERSION 3.0)
project(kcpss)

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined,vptr -fsanitize-recover=address -fsanitize-address-use-after-scope")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fno-omit-frame-pointer")

# set coverages ===============================================================

#if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
#  add_compile_options(-fprofile-arcs -ftest-coverage)
#  add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
#  add_link_options(-fprofile-instr-generate -fcoverage-mapping)
#  #Uncomment in case of linker errors
#  #link_libraries(clang_rt.profile-x86_64)
#elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#  add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
#  #Uncomment in case of linker errors
#  link_libraries(gcov)
#endif ()

# set colors ==================================================================
if (NOT WIN32)
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(ColourBold "${Esc}[1m")
  set(Red "${Esc}[31m")
  set(Green "${Esc}[32m")
  set(Yellow "${Esc}[33m")
  set(Blue "${Esc}[34m")
  set(Magenta "${Esc}[35m")
  set(Cyan "${Esc}[36m")
  set(White "${Esc}[37m")
  set(BoldRed "${Esc}[1;31m")
  set(BoldGreen "${Esc}[1;32m")
  set(BoldYellow "${Esc}[1;33m")
  set(BoldBlue "${Esc}[1;34m")
  set(BoldMagenta "${Esc}[1;35m")
  set(BoldCyan "${Esc}[1;36m")
  set(BoldWhite "${Esc}[1;37m")
endif ()

include_directories(/usr/local/include)
include_directories(depends/kcp)
include_directories(depends/snappy)

link_directories(/usr/local/lib)

file(GLOB_RECURSE kcp_sources ./source/*.cpp ./source/*.h)
file(GLOB_RECURSE depend_sources
    depends/*.h
    depends/kcp/*.c
    depends/snappy/*.cc)

link_libraries(ev pthread)

add_executable(kcpss ${kcp_sources} ${depend_sources})

find_program(CLANG_FORMAT_BIN NAMES clang-format)
if (EXISTS ${CLANG_FORMAT_BIN})
  MESSAGE(STATUS "Clang-format enable")
  add_custom_target(
      clang-format ALL
      COMMAND clang-format -i -style=file ${kcp_sources} ${depend_sources}
  )
  add_dependencies(kcpss clang-format)
endif ()