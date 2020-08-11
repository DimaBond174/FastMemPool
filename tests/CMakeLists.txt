#  This is test_exe builder

set(SPEC_SRC_COMMON
      ${CMAKE_CURRENT_SOURCE_DIR}/common
      )

# OS configuration
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  message(STATUS "Linux was chosen ")
  set(OS_INCLUDE
    ${OS_INCLUDE}
    ${SPEC_SRC_COMMON}/system/linux
       )

  set(OS_SRC
    ${OS_SRC}
    ${SPEC_SRC_COMMON}/system/linux/linuxsystem.cpp
     )

  set(OS_LIBS
             dl
            pthread
            stdc++fs
         )

elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
   message(STATUS "Windows was chosen ")

set(OS_INCLUDE
  ${OS_INCLUDE}
  ${SPEC_SRC_COMMON}/system/windows
     )

set(OS_SRC
  ${OS_SRC}
  ${SPEC_SRC_COMMON}/system/windows/windowssystem.cpp
   )

   set(OS_DEFINITIONS
    "WIN32_LEAN_AND_MEAN"
    FD_SETSIZE=1024
  )
endif()
message("OS_SRC : ${OS_SRC}")


set(EXE_NAME
  tester.exe
)

set(EXE_INCLUDE
    ${SPEC_INCLUDE}
    ${OS_INCLUDE}
    ${SPEC_SRC_COMMON}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_exe
    )

file(GLOB_RECURSE EXE_SRC
       ${CMAKE_CURRENT_SOURCE_DIR}/test_exe/*.cpp
   )

set(EXE_SRC
    ${EXE_SRC}
    ${OS_SRC}
)
message("EXE_SRC : ${EXE_SRC}")

custom_add_executable(${EXE_NAME}
    "${SPEC_BUILD_DIR}"
    "${EXE_SRC}"
    "${EXE_INCLUDE}"
    "${SPEC_DEFINITIONS}"
    "${OS_LIBS}"
    "${SPEC_PROPERTIES}"
    )

#==============================
# Overhead test
set(EXE2_NAME
  test_overhead.exe
)

set(EXE2_INCLUDE
    ${SPEC_INCLUDE}
    ${OS_INCLUDE}
    ${SPEC_SRC_COMMON}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_overhead
    )

file(GLOB_RECURSE EXE2_SRC
       ${CMAKE_CURRENT_SOURCE_DIR}/test_overhead/*.cpp
   )

set(EXE2_SRC
    ${EXE2_SRC}
    ${OS_SRC}
)
message("EXE2_SRC : ${EXE2_SRC}")

custom_add_executable(${EXE2_NAME}
    "${SPEC_BUILD_DIR}"
    "${EXE2_SRC}"
    "${EXE2_INCLUDE}"
    "${SPEC_DEFINITIONS}"
    "${OS_LIBS}"
    "${SPEC_PROPERTIES}"
    )
