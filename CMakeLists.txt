cmake_minimum_required(VERSION 3.10)
project(IceTJNIBridge CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(JNI REQUIRED)
if (JNI_FOUND)
    message(STATUS "Found JNI at: ${JNI_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "Could not find JNI.")
endif()

find_package(MPI REQUIRED)
if (MPI_FOUND)
    message(STATUS "Found MPI: ${MPI_CXX_LIBRARIES}")
else()
    message(FATAL_ERROR "Could not find MPI.")
endif()

find_package(IceT REQUIRED)

message(STATUS "Using IceT include dir: ${ICET_INCLUDE_DIRS}")

include_directories(
        ${JNI_INCLUDE_DIRS}
        ${MPI_CXX_INCLUDE_PATH}
        ${ICET_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME} SHARED
        IcetContext.cpp
        IcetJNI.cpp
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        MPI::MPI_CXX         # or ${MPI_CXX_LIBRARIES}
        ${ICET_CORE_LIBS} ${ICET_GL_LIBS} ${ICET_MPI_LIBS}
        ${JNI_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "IceTJNIBridge"
)

install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)
