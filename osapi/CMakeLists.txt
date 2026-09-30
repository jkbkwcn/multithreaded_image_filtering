# project(osapi C)

# if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
#     message(STATUS "Building for Linux")
#     add_library(osapi STATIC 
#         linux/osapi_linux.cpp
#     )
#     target_include_directories(osapi PUBLIC ${CMAKE_CURRENT_LIST_DIR} PRIVATE linux)
# elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
#     message(STATUS "Building for Windows")
#     add_library(osapi STATIC 
#         windows/osapi_windows.cpp
#     )
#     target_include_directories(osapi PUBLIC ${CMAKE_CURRENT_LIST_DIR} PRIVATE windows)
# elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
#     message(STATUS "Building for an unsupported system: ${CMAKE_SYSTEM_NAME}")
# endif ()

add_library(osapi INTERFACE)

target_include_directories(osapi INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/windows
)