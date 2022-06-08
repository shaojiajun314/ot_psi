#project(PSISO)

#############################################
#               Build PSISO              #
#############################################

file(GLOB_RECURSE SRCS_TEST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)


# add_executable(PSISO ${SRCS_TEST})
# add_compile_options(-fPIC)
add_library(PSISO SHARED ${SRCS_TEST})

#############################################
#            Link our libraries             #
#############################################
target_link_libraries(PSISO PSI)
