cmake_minimum_required(VERSION 4.0.0)
project(TokenDaemon VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)

file(GLOB SRC_FILES src/*.cpp)

message(STATUS "Source files: ${SRC_FILES}")

# Add the ucrt directory for linking
set(UCRT_DIR "C:/msys64/ucrt64")

# Console executable
add_executable(${PROJECT_NAME}_console ${SRC_FILES})
target_include_directories(${PROJECT_NAME}_console PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${UCRT_DIR}/include
)
target_link_directories(${PROJECT_NAME}_console PUBLIC ${UCRT_DIR}/lib)
target_link_libraries(${PROJECT_NAME}_console PUBLIC
    ${UCRT_DIR}/lib/libcurl.dll.a
    ${UCRT_DIR}/lib/libssl.dll.a
    ${UCRT_DIR}/lib/libcrypto.dll.a
)

# Daemon executable (no command prompt)
add_executable(${PROJECT_NAME}_daemon ${SRC_FILES})
set_target_properties(${PROJECT_NAME}_daemon PROPERTIES WIN32_EXECUTABLE ON)
target_include_directories(${PROJECT_NAME}_daemon PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${UCRT_DIR}/include
)
target_link_directories(${PROJECT_NAME}_daemon PUBLIC ${UCRT_DIR}/lib)
target_link_libraries(${PROJECT_NAME}_daemon PUBLIC
    ${UCRT_DIR}/lib/libcurl.dll.a
    ${UCRT_DIR}/lib/libssl.dll.a
    ${UCRT_DIR}/lib/libcrypto.dll.a
)
