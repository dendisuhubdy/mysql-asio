cmake_minimum_required(VERSION 3.11.0)
project(mysql-asio)

# If we are the top-level project, enable testing
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
    if(BUILD_TESTING)
    	set(_MYSQL_TESTING_ENABLED ON)
    endif()
endif()

# Utility function to set warnings and other compile properties of
# our test targets
function(_mysql_set_target_warnings TARGET_NAME)
	if(MSVC)
		target_compile_definitions(${TARGET_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
	else()
		target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
	endif()
	set_target_properties(${TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF) # disable extensions
endfunction()

set(_MYSQL_SHELL_EXT $<IF:$<PLATFORM_ID:Windows>,bat,sh>)

# Dependencies
include(FetchContent)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Date
FetchContent_Declare(
    date
    GIT_REPOSITORY https://github.com/HowardHinnant/date.git
    GIT_TAG        v2.4.1
)

FetchContent_GetProperties(date)
if(NOT date_POPULATED)
    FetchContent_Populate(date)
    # Building date requires CMake 3.14, and we only need headers
endif()

# Interface library (header-only)
add_library(
    mysql_asio
    INTERFACE
)

target_link_libraries(
    mysql_asio
	INTERFACE
	Boost::system
	Threads::Threads
	OpenSSL::Crypto
)
target_include_directories(
	mysql_asio
	INTERFACE
	include
	${date_SOURCE_DIR}/include
)
target_compile_features(
	mysql_asio
	INTERFACE
	cxx_std_17
)

# Examples
if(_MYSQL_TESTING_ENABLED)
	add_subdirectory(examples)
endif()

# Tests
if(_MYSQL_TESTING_ENABLED)
	add_subdirectory(test)
endif()
