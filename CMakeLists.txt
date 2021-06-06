cmake_minimum_required(VERSION 3.17)
project(ModuleRunner)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Boost)
IF (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
endif ()
set(Boost_USE_STATIC_LIBS OFF) # enable dynamic linking
set(Boost_USE_MULTITHREAD ON)  # enable multithreading

function(library_from_folder NAME )
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src")
        string(TOUPPER ${NAME} UPPERNAME)
        file(GLOB ${UPPERNAME}_SOURCE "src/*.cpp")
        file(GLOB ${UPPERNAME}_HEADERS "include/*.h")
        add_library(${NAME} ${${UPPERNAME}_SOURCE} ${${UPPERNAME}_HEADERS})
        target_include_directories(${NAME} PUBLIC include)
    else ()
        add_library(${NAME} INTERFACE IMPORTED GLOBAL)
        target_include_directories(${NAME} INTERFACE include/)
    endif ()
endfunction()

library_from_folder(module_runner)

add_subdirectory(examples)