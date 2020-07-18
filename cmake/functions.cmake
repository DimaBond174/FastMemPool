#  This is the source code of SpecNet project
#  It is licensed under MIT License.
#  Copyright (c) Dmitriy Bondarenko
#  feel free to contact me: specnet.messenger@gmail.com

function(custom_enable_cxx17 TARGET)
    message("custom_enable_cxx17 CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
    if (CMAKE_CXX_COMPILER STREQUAL "clang++")
      message("custom_enable_cxx17: clang++: ${TARGET}")
#      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -v")
#      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
        if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-std=c++17  -stdlib=libc++ -g -O0")
            set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-stdlib=libc++ -lc++abi")
        else()
            set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-std=c++17  -stdlib=libc++")
            set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-stdlib=libc++ -lc++abi")
        endif ()

        target_link_libraries(${TARGET}
            dl
            )
    #elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
        message("custom_enable_cxx17 MSVC: ${TARGET}")
        set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "/std:c++latest")
    else()
      message("custom_enable_cxx17 GCC: ${TARGET}")
      set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-std=c++17 -pthread -Wall -pedantic")
      target_link_libraries(${TARGET}
          dl
          )
    endif()
endfunction(custom_enable_cxx17)

function(custom_enable_cxx17libc TARGET)
    if (CMAKE_CXX_COMPILER STREQUAL "clang++")
#        set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-stdlib=libc++ -pthread")
        set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-stdlib=libc++ -pthread -Wall -pedantic")
#        set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-I${CLANGPATH}/include/c++/v1")
        target_include_directories(${TARGET} PRIVATE "${CLANG_PATH}/include/c++/v1")
        target_link_libraries(${TARGET}
            c++experimental
            c++
            c++abi)
    endif()
endfunction(custom_enable_cxx17libc)


# Add custom sources:
#function(custom_add_library_from_dir TARGET)
#    file(GLOB TARGET_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
#    add_library(${TARGET} ${TARGET_SRC})
#endfunction()

# Add TARGET - library:
function(custom_add_library_from_dir TARGET)
    file(GLOB TARGET_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
    add_library(${TARGET}
        SHARED
        ${TARGET_SRC})
    if (CMAKE_CXX_COMPILER STREQUAL "clang++")
        set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-std=c++17 -shared -fPIC")
        target_link_libraries(${TARGET}
            dl
            )
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "/std:c++latest")
    endif()
endfunction()


# Add TARGET - executive:
function(custom_add_executable_from_dir TARGET)
    file(GLOB TARGET_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
    add_executable(${TARGET} ${TARGET_SRC})
endfunction()

#Add TARGET  - executive with all properties
function(custom_add_executable TARGET
        TARGET_BUILD_DIR
        TARGET_SRC
        TARGET_INCLUDES
        TARGET_DEFINITIONS
        TARGET_LINK_LIBS
        TARGET_PROPERTIES
        )
    message(STATUS "custom_add_executable: ${TARGET}")
    # Output folder for binaries
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  ${TARGET_BUILD_DIR})
   # set(CMAKE_BINARY_DIR  ${SPEC_BUILD_DIR})
    #link_directories(${TARGET_BUILD_DIR}/Debug)

    message(STATUS "TARGET_SRC: ${TARGET_SRC}")

    add_executable(${TARGET} ${TARGET_SRC})
    custom_enable_cxx17(${TARGET})
    message(STATUS "include_directories: ${TARGET_INCLUDES}")
    target_include_directories(${TARGET} PRIVATE ${TARGET_INCLUDES})
#    target_compile_definitions(${TARGET} PRIVATE ${TARGET_DEFINITIONS})
    message(STATUS "compile_definitions: ${TARGET_DEFINITIONS}")
    target_compile_definitions(${TARGET} PUBLIC ${TARGET_DEFINITIONS})
    message(STATUS "link_libraries: ${TARGET_LINK_LIBS}")
    target_link_libraries(${TARGET}  ${TARGET_LINK_LIBS}  )
   # find_library(testlib1_LIBRARY testlib1 HINTS ${TARGET_BUILD_DIR})
   # message("testlib1_LIBRARY : ${testlib1_LIBRARY}")
   # target_link_libraries(${TARGET} PUBLIC ${testlib1_LIBRARY})

    message(STATUS "target_properties: ${TARGET_PROPERTIES}")
    if (NOT("${TARGET_PROPERTIES}" STREQUAL ""))
        set_target_properties(${TARGET}
            PROPERTIES
            ${TARGET_PROPERTIES}
        )
    endif()
endfunction()

#Add TARGET  - lib with all properties
function(custom_add_lib TARGET
    TARGET_BUILD_DIR
    TARGET_SRC
    TARGET_INCLUDES
    TARGET_DEFINITIONS
    TARGET_LINK_LIBS
    TARGET_PROPERTIES
    )
  message(STATUS "custom_add_lib: ${TARGET}")
#    file(GLOB ADD_TARGET_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
#    set(ADD_TARGET_SRC ${ADD_TARGET_SRC} ${TARGET_SRC})
#    set(ADD_TARGET_INCLUDES ${TARGET_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  ${TARGET_BUILD_DIR})

  add_library(${TARGET}
        SHARED
        ${TARGET_SRC}
        )
  custom_enable_cxx17(${TARGET})
#    if (CMAKE_CXX_COMPILER STREQUAL "clang++")
#        if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
#            set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-std=c++17 -fPIC -rdynamic -shared -g -O0")
#        else()
#            set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-std=c++17 -fPIC -rdynamic -shared ")
#        endif ()


#        target_link_libraries(${TARGET}
#            dl
#            )
#    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
#        set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "/std:c++latest")
#    endif()

##    custom_enable_cxx17(${TARGET})
#    message(STATUS "include_directories: ${TARGET_INCLUDES}")
#    target_include_directories(${TARGET} PRIVATE ${ADD_TARGET_INCLUDES})
##    target_compile_definitions(${TARGET} PRIVATE ${TARGET_DEFINITIONS})
#    message(STATUS "compile_definitions: ${TARGET_DEFINITIONS}")
#    target_compile_definitions(${TARGET} PUBLIC ${TARGET_DEFINITIONS})
#    message(STATUS "link_libraries: ${TARGET_LINK_LIBS}")
#    target_link_libraries(${TARGET}  ${TARGET_LINK_LIBS}  )
#    message(STATUS "target_properties: ${TARGET_PROPERTIES}")
    message(STATUS "include_directories: ${TARGET_INCLUDES}")
    target_include_directories(${TARGET} PRIVATE ${TARGET_INCLUDES})
##    message(STATUS "compile_definitions: ${TARGET_DEFINITIONS}")
    target_compile_definitions(${TARGET} PUBLIC ${TARGET_DEFINITIONS})
##    message(STATUS "link_libraries: ${TARGET_LINK_LIBS}")
    target_link_libraries(${TARGET}  ${TARGET_LINK_LIBS}  )
##    message(STATUS "target_properties: ${TARGET_PROPERTIES}")
  if (NOT("${TARGET_PROPERTIES}" STREQUAL ""))
    message(STATUS "target_properties: ${TARGET_PROPERTIES}")
    set_target_properties(${TARGET}
        PROPERTIES
        ${TARGET_PROPERTIES}
    )
  endif()
endfunction()

#Add TARGET  - lib with all properties
function(custom_add_static_lib TARGET
        TARGET_BUILD_DIR
        TARGET_SRC
        TARGET_INCLUDES
        TARGET_DEFINITIONS
        TARGET_LINK_LIBS
        )
    message(STATUS "custom_add_static_lib: ${TARGET}")

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  ${TARGET_BUILD_DIR})

    add_library(${TARGET}
        ${TARGET_SRC}
        )
    custom_enable_cxx17(${TARGET})
#    message(STATUS "include_directories: ${TARGET_INCLUDES}")
    target_include_directories(${TARGET} PRIVATE ${TARGET_INCLUDES})
##    message(STATUS "compile_definitions: ${TARGET_DEFINITIONS}")
    target_compile_definitions(${TARGET} PUBLIC ${TARGET_DEFINITIONS})
##    message(STATUS "link_libraries: ${TARGET_LINK_LIBS}")
    target_link_libraries(${TARGET}  ${TARGET_LINK_LIBS}  )
##    message(STATUS "target_properties: ${TARGET_PROPERTIES}")

endfunction()

#Add TARGET  - lib with all properties
function(custom_add_lib_android TARGET
    TARGET_ABI
    TARGET_BUILD_DIR
    TMP_BUILD_DIR
    TARGET_PREREQ
    TARGET_SRC
    TARGET_INCLUDES
    TARGET_DEFINITIONS
    TARGET_LINK_LIBS
    TARGET_PROPERTIES
    )
  message(STATUS "custom_add_lib_android: ${TARGET}")
  if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
      set(TARGET_BUILD_TYPE "Debug")
  else()
      set(TARGET_BUILD_TYPE "Release")
  endif()

  set(TMP_BUILD_DIR_ABI "${TMP_BUILD_DIR}/${TARGET}/${TARGET_ABI}/${TARGET_BUILD_TYPE}" )
  set(TARGET_BUILD_DIR_ABI "${TARGET_BUILD_DIR}/${TARGET_ABI}" )
  #надо? file(REMOVE_RECURSE  "${TMP_BUILD_DIR_ABI}")
  #https://cmake.org/cmake/help/latest/command/file.html
  if(NOT EXISTS "${TMP_BUILD_DIR_ABI}")
    file(MAKE_DIRECTORY ${TMP_BUILD_DIR_ABI})
  endif()

  message(STATUS "TMP_BUILD_DIR_ABI: ${TMP_BUILD_DIR_ABI}")
  set(TMP_BUILD_ABI_CMake_file "${TMP_BUILD_DIR_ABI}/CMakeLists.txt" )
  set(TMP_BUILD_ABI_CMake_exec "${TMP_BUILD_DIR_ABI}/build.sh" )
  file(REMOVE "${TMP_BUILD_ABI_CMake_file}")
  file(REMOVE "${TMP_BUILD_ABI_CMake_exec}")

  #create CMakeLists.txt with absolute paths to SRC's and other
#  file(APPEND "${TMP_BUILD_ABI_CMake_file}"
 #   "curl --create-dirs --output \"${CACHE_CANDIDATE}\" \"${DL_URL}\"\n")
#  set(TMP_BUILD_ABI_CMake_file_content
#    "cmake_minimum_required(VERSION 3.6 FATAL_ERROR)"
#    "set (LIB_plugin_cam_SRC"
#      ${TARGET_SRC}
#    ")"
#    ) #set
#------------------------- Create CMake ---------------------------------------
  file(APPEND "${TMP_BUILD_ABI_CMake_file}" "cmake_minimum_required(VERSION 3.6 FATAL_ERROR)\n"
    #"set (LIB_SRC \n ${TARGET_SRC} \n)"
    #"set (LIB_INCLUDES \n ${TARGET_INCLUDES} \n)"
    "set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR_ABI})\n"
    "set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR_ABI})\n"
    "set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TMP_BUILD_DIR_ABI})\n"
    "set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  ${TMP_BUILD_DIR_ABI})\n"
    "${TARGET_PREREQ}\n"
    "add_library(${TARGET}  SHARED  ${TARGET_SRC}  ) \n"
    "set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS \"-std=c++17  -stdlib=libc++ -g -O0\")\n"
    "set_target_properties(${TARGET} PROPERTIES LINK_FLAGS \"-stdlib=libc++ -lc++abi\")\n"
    "target_include_directories(${TARGET} PRIVATE ${TARGET_INCLUDES})\n"
     )
   if (NOT("${TARGET_DEFINITIONS}" STREQUAL ""))
     file(APPEND "${TMP_BUILD_ABI_CMake_file}"
       "target_compile_definitions(${TARGET} PUBLIC ${TARGET_DEFINITIONS})\n"  )
   endif()
   if (NOT("${TARGET_LINK_LIBS}" STREQUAL ""))
     file(APPEND "${TMP_BUILD_ABI_CMake_file}"
       "target_link_libraries(${TARGET}  ${TARGET_LINK_LIBS}  )\n"  )
   endif()
   if (NOT("${TARGET_PROPERTIES}" STREQUAL ""))
     file(APPEND "${TMP_BUILD_ABI_CMake_file}"
       "set_target_properties(${TARGET}  PROPERTIES  ${TARGET_PROPERTIES} )\n"
       )
   endif()

#------------------------- Create Builder ---------------------------------------
  file(APPEND "${TMP_BUILD_ABI_CMake_exec}"
    "#!/bin/bash\n"
#    "cd ${TMP_BUILD_DIR_ABI}\n"
    "mkdir CMakeWorkDir_toDelete\n"
    "cd CMakeWorkDir_toDelete\n"
    "cmake -DANDROID_ABI=${TARGET_ABI} -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake  -DANDROID_NATIVE_API_LEVEL=16 -DCMAKE_BUILD_TYPE=${TARGET_BUILD_TYPE} ..\n"
    "cmake --build .\n"
    )

#  set(BUILD_STRING
#    " -DANDROID_ABI=${TARGET_ABI} -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake  -DANDROID_NATIVE_API_LEVEL=16 -DCMAKE_BUILD_TYPE=${TARGET_BUILD_TYPE} .."
#    )
#------------------------- Build ---------------------------------------
# !!! Сам CMake умеет  кучу команд делать когда вызываешь его с ключом -E:
#execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CACHE_CANDIDATE}" "${COPY_DESTINATION}"
#                RESULT_VARIABLE res)
#execute_process(COMMAND ${CMAKE_COMMAND} "${BUILD_STRING}"
#              WORKING_DIRECTORY "${TMP_BUILD_DIR_ABI_BUILD}"
#              RESULT_VARIABLE res)
#            поэкспериментируй с самой CMake -E = должен уметь билдить
#
#https://stackoverflow.com/questions/35689501/cmakes-execute-process-and-arbitrary-shell-scripts
#chmod +x ./build.sh
#message("TMP_BUILD_ABI_CMake_exec : ${TMP_BUILD_ABI_CMake_exec}")
#execute_process(COMMAND chmod "+x" "${TMP_BUILD_ABI_CMake_exec}"
#                WORKING_DIRECTORY "${TMP_BUILD_DIR_ABI}"
#                RESULT_VARIABLE res)

#execute_process(COMMAND bash "-c" "${TMP_BUILD_ABI_CMake_exec}"
#                WORKING_DIRECTORY "${TMP_BUILD_DIR_ABI}"
#                RESULT_VARIABLE res)
#execute_process(
#                  COMMAND bash "-c" "${BUILD_STRING}"
# виснет                 OUTPUT_VARIABLE res
#              )
#message("COMMAND bash res = ${res}")

#execute_process(
#    COMMAND bash "-c" "echo -n hello | sed 's/hello/world/;'"
#    OUTPUT_VARIABLE FOO
#)
#message("COMMAND bash FOO = ${FOO}")

#  set(CMAKE_SYSTEM_NAME Android)
#  set(CMAKE_SYSTEM_VERSION 16) # API level
#  set(CMAKE_ANDROID_ARCH_ABI "${TARGET_ABI}")#arm64-v8a)
#   message(STATUS "CMAKE_ANDROID_ARCH_ABI: ${CMAKE_ANDROID_ARCH_ABI}")
#  set(CMAKE_ANDROID_NDK "${ANDROID_NDK}")
#  message(STATUS "ANDROID_NDK: ${ANDROID_NDK}")
#  set(CMAKE_ANDROID_STL_TYPE gnustl_static)

#  set(CMAKE_SYSTEM_NAME Android)
#  set(CMAKE_SYSTEM_VERSION 22)
#  set(CMAKE_ANDROID_ARM_MODE ON)
#  set(CMAKE_ANDROID_ARM_NEON ON)
#  set(CMAKE_ANDROID_ARCH_ABI "${TARGET_ABI}")
#  set(CMAKE_ANDROID_NDK "${ANDROID_NDK}")
#  set(CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION clang)
#  set(CMAKE_ANDROID_STL_TYPE c++_static)
#  set(CMAKE_SHARED_LINKER_FLAGS -static-libstdc++)

#  add_definitions(-DANDROID)

#  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
##    file(GLOB ADD_TARGET_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
##    set(ADD_TARGET_SRC ${ADD_TARGET_SRC} ${TARGET_SRC})
##    set(ADD_TARGET_INCLUDES ${TARGET_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR})
#  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR}/libs)
#  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR}/libs)
#  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TARGET_BUILD_DIR})
#  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  ${TARGET_BUILD_DIR})

#  add_library(${TARGET}
#        SHARED
#        ${TARGET_SRC}
#        )
## custom_enable_cxx17(${TARGET})
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

#    message(STATUS "include_directories: ${TARGET_INCLUDES}")
#    target_include_directories(${TARGET} PRIVATE ${TARGET_INCLUDES})
###    message(STATUS "compile_definitions: ${TARGET_DEFINITIONS}")
#  set(TARGET_ANDROID_DEFINITIONS
#    ${TARGET_DEFINITIONS}
#    "ANDROID_ABI=${TARGET_ABI}"
#    "CMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake"
#    "ANDROID_NATIVE_API_LEVEL=16"
#    )
#  message(STATUS "TARGET_ANDROID_DEFINITIONS: ${TARGET_ANDROID_DEFINITIONS}")
#    target_compile_definitions(${TARGET} PUBLIC ${TARGET_ANDROID_DEFINITIONS})
###    message(STATUS "link_libraries: ${TARGET_LINK_LIBS}")
#    target_link_libraries(${TARGET}  ${TARGET_LINK_LIBS}  )
###    message(STATUS "target_properties: ${TARGET_PROPERTIES}")
#  if (NOT("${TARGET_PROPERTIES}" STREQUAL ""))
#    message(STATUS "target_properties: ${TARGET_PROPERTIES}")
#    set_target_properties(${TARGET}
#        PROPERTIES
#        ${TARGET_PROPERTIES}
#    )
#  endif()
endfunction() #add_lib_android
