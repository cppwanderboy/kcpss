cmake_minimum_required(VERSION 3.0)
project(kcpss)

# version info ================================================================
set(kcpss_VERSION_MAJOR 0)
set(kcpss_VERSION_MINOR 1)
set(kcpss_VERSION_PATCH 0)

string(TIMESTAMP BUILDTIME "%Y%m%d-%H:%M:%S")
set(kcpss_VERSION ${kcpss_VERSION_MAJOR}.${kcpss_VERSION_MINOR}.${kcpss_VERSION_PATCH})
set(kcpss_SO_VERSION ${kcpss_VERSION_MAJOR}.${kcpss_VERSION_MINOR})
set(kcpss_BUILD_VERSION ${kcpss_VERSION}-${BUILDTIME})

# Get the current working branch
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the latest abbreviated commit hash of the working branch
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the latest commit subject of the working branch
execute_process(
    COMMAND git log -1 --format=%s
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_MESSAGE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

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

configure_file(source/config.h.in ${CMAKE_SOURCE_DIR}/source/config.h)
include_directories(/usr/local/include)
include_directories(depends/kcp)
include_directories(depends/ini)
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

# install =====================================================================
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)
install(TARGETS kcpss DESTINATION bin)
install(FILES bin/kcpss.ini DESTINATION etc)

# packing =====================================================================
set(CPACK_PACKAGE_NAME "kcpss")
set(CPACK_PACKAGE_VENDOR "Gui Yang(yanggui1989@gmail.com)")
set(CPACK_PACKAGE_CONTACT "Gui Yang(yanggui1989@gmail.com)")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "kcpss - a fast socks5 proxy")
set(CPACK_PACKAGE_VERSION ${kcpss_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${kcpss_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${kcpss_VERSION_MINOR})
#set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CPack Component Example")

set(CPACK_CMAKE_GENERATOR "Unix Makefiles")
set(CPACK_GENERATOR "TGZ;RPM;DEB")
set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_SOURCE_DIR}/ChangeLog.txt")
set(CPACK_RPM_PACKAGE_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
set(CPACK_RPM_PACKAGE_GROUP "Applications/Internet")
set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION_SUMMARY} ${RELEASE_DESCRIB})

include(CPack)
