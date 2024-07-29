# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(EASE
        [VERSION 1.0.2]
        [LANGUAGE C]
        )

# set(compiler_defs )
# list(APPEND compiler_defs
#         "-Wno-unused-const-variable" 
#         "-Wunused-variable")

# set_property( TARGET  APPEND_STRING  PROPERTY  <EXTRA_PARTITION_SUBTYPES> <data>,<fact_metas>,<20> )

# idf_build_set_property(EXTRA_PARTITION_SUBTYPES APPEND "meta_data,fact_meta,4")

# get_property(new_var GLOBAL PROPERTY EXTRA_PARTITION_SUBTYPES)

# message("the property is ${new_var}")

idf_build_set_property(COMPILE_OPTIONS "-Wall"  APPEND)

# idf_build_set_property(COMPILE_OPTIONS "-Werror"  APPEND)
# ### compiler options to avoid compile time errors for unused variables 
# target_compile_options(${COMPONEN} PUBLIC ${compiler_defs})


