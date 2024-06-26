cmake_minimum_required (VERSION 3.4)

project(libOTe)


if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")

	############################################
	#          If top level cmake              #
	############################################

	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin)
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib)

	############################################
	#           Flag and #defines              #
	############################################
	add_definitions(-DSOLUTION_DIR='${CMAKE_SOURCE_DIR}')
	set(CMAKE_C_FLAGS "-ffunction-sections -Wall  -maes -msse2 -msse4.1 -mpclmul -Wfatal-errors -pthread -Wno-strict-overflow  -fPIC -Wno-ignored-attributes -Wno-parentheses")
	# set(CMAKE_CXX_FLAGS  "${CMAKE_C_FLAGS}  -std=c++14")
	SET(CMAKE_CXX_FLAGS "-fPIC")
	# Select flags.
	SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native  -DNDEBUG")
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO " -O2 -g -ggdb -rdynamic")
	SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g -ggdb -rdynamic")


	############################################
	#           Build mode checks              #
	############################################

	# Set a default build type for single-configuration
	# CMake generators if no build type is set.
	if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	   SET(CMAKE_BUILD_TYPE Release)
	endif()

	if(    NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release"
       AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug"
       AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "RELWITHDEBINFO" )

        message(FATAL_ERROR ": Unknown build type - ${CMAKE_BUILD_TYPE}  Please use one of Debug, Release, or RELWITHDEBINFO\n\n" )
	endif()

    message(STATUS "Option: CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}\n\tRelease\n\tDebug\n\tRELWITHDEBINFO")

endif()


#############################################
#                CONFIGURE                  #
#############################################


option(ENABLE_SIMPLESTOT "Build the assembly based SimplestOT library" OFF)

message(STATUS "Option: ENABLE_SIMPLESTOT = ${ENABLE_SIMPLESTOT}")

configure_file(libOTe/config.h.in libOTe/config.h)

#############################################
#    Build cryptoTools (common utilities)   #
#############################################
#include_directories(cryptoTools)
add_subdirectory(cryptoTools)


#############################################
#               Build libOTe                #
#############################################

if(ENABLE_SIMPLESTOT)
	add_subdirectory(SimplestOT)
endif(ENABLE_SIMPLESTOT)

add_subdirectory(libOTe)
add_subdirectory(libOTe_Tests)

#############################################
#              Build Frontend               #
#############################################
add_subdirectory(frontend)
