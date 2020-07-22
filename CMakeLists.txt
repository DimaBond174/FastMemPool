#  This is the source code of SpecNet project
#  It is licensed under MIT License.
#  Copyright (c) Dmitriy Bondarenko
#  feel free to contact me: specnet.messenger@gmail.com

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(fast_mem_pool)

set(Proj_VERSION  "1.0")


# TODO Set build type:
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type Debug|Release")
message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# TODO Set in code unit test:
option(Provide_inline_unit_tests "Provide inline unit tests" ON)
message("Provide_inline_unit_tests: ${Provide_inline_unit_tests}")


# TODO Needto build Tests :
option(CREATE_TESTS "Create tests" ON)

# TODO Set bin folder:
set(PROJ_EXEC_BIN_FOLDER "../fast_mem_pool_bin" CACHE PATH "Binary output folder")
message("PROJ_EXEC_BIN_FOLDER: ${PROJ_EXEC_BIN_FOLDER}")

# TODO Set project defaults:
set(DEF_Leaf_Size_Bytes "65536" CACHE PATH "Size of each memory pool leaf")
message("DEF_Leaf_Size_Bytes: ${DEF_Leaf_Size_Bytes}")

set(DEF_Leaf_Cnt "16" CACHE PATH "Memory pool leaf count")
message("DEF_Leaf_Cnt: ${DEF_Leaf_Cnt}")

set(DEF_Average_Allocation "512" CACHE PATH "Average allocation from malloc")
message("DEF_Average_Allocation: ${DEF_Average_Allocation}")

option(DEF_Need_Registry "Need registry of all allocations" OFF)
option(DEF_Raise_Exeptions "Need raise exeptions on Errors" ON)
option(DEF_Do_OS_malloc "If this memory pool depleted go do OS malloc" ON)


set(SPEC_PROPERTIES
    DEBUG_POSTFIX             "d"
)
# TODO Enshure target build is what you want:
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(SPEC_BUILD DDEBUG)
else()
    set(SPEC_BUILD DRELEASE)
endif()

# Output folder for binaries
set(SPEC_BUILD_DIR_REL ${CMAKE_CURRENT_SOURCE_DIR}/${PROJ_EXEC_BIN_FOLDER})
get_filename_component(SPEC_BUILD_DIR "${SPEC_BUILD_DIR_REL}"
                       REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
message("SPEC_BUILD_DIR : ${SPEC_BUILD_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SPEC_BUILD_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SPEC_BUILD_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SPEC_BUILD_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  ${SPEC_BUILD_DIR})
#set(CMAKE_BINARY_DIR  ${SPEC_BUILD_DIR})

#set(SPEC_BUILD_DIR_REL ${CMAKE_CURRENT_SOURCE_DIR}/${PROJ_TEMP_BINARY_DIR})
#get_filename_component(CMAKE_BINARY_DIR "${SPEC_BUILD_DIR_REL}"
#                      REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
#message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")


include(cmake/functions.cmake)

set(SPEC_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

set(SPEC_SRC_COMMON
      ${CMAKE_CURRENT_SOURCE_DIR}/src/common
      )

set(SPEC_LIB_DIR
          ${CMAKE_CURRENT_SOURCE_DIR}/static
          )

  # Configuration of the assembly
  #   according to the selected components:
set(SPEC_DEFINITIONS
      ${CMAKE_SYSTEM_NAME}
      ${CMAKE_BUILD_TYPE}
      ${SPEC_BUILD}
      SPEC_VERSION="${Proj_VERSION}"
      DEF_Leaf_Size_Bytes=${DEF_Leaf_Size_Bytes}
      DEF_Leaf_Cnt=${DEF_Leaf_Cnt}
      DEF_Average_Allocation=${DEF_Average_Allocation}
      DEF_Need_Registry=${DEF_Need_Registry}
  )

if (DEF_Need_Registry)
  set(SPEC_DEFINITIONS ${SPEC_DEFINITIONS}
    DEF_Need_Registry=true)
else()
  set(SPEC_DEFINITIONS ${SPEC_DEFINITIONS}
    DEF_Need_Registry=false)
endif()

if (DEF_Raise_Exeptions)
  set(SPEC_DEFINITIONS ${SPEC_DEFINITIONS}
    DEF_Raise_Exeptions=true)
else()
  set(SPEC_DEFINITIONS ${SPEC_DEFINITIONS}
    DEF_Raise_Exeptions=false)
endif()

if (DEF_Do_OS_malloc)
  set(SPEC_DEFINITIONS ${SPEC_DEFINITIONS}
    DEF_Do_OS_malloc=true)
else()
  set(SPEC_DEFINITIONS ${SPEC_DEFINITIONS}
    DEF_Do_OS_malloc=false)
endif()

if (Provide_inline_unit_tests)
  message("will compile with Provide_inline_unit_tests")
  set(SPEC_DEFINITIONS
    ${SPEC_DEFINITIONS}
    Provide_inline_unit_tests
    )
endif()

message("SPEC_DEFINITIONS: ${SPEC_DEFINITIONS}")


message(STATUS "Building Tests..")
add_subdirectory(tests)


