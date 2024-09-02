cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(APPLICATION_VERSION 1.1.0)

project(ease_app)

# after succesfull building of the application need to run the python 
# for modifying the changes in the file 

idf_build_get_property(python PYTHON)
idf_build_get_property(build_dir BUILD_DIR)


set(python_file  ${CMAKE_CURRENT_SOURCE_DIR}/python/app_hdr_gen.py)

add_custom_command(TARGET app 
                    POST_BUILD                    
                    COMMAND ${CMAKE_COMMAND} -E echo "modifying ${PROJECT_NAME} header with python "
                    COMMAND ${CMAKE_COMMAND} -E env "APPLICATION_VERSION=${APPLICATION_VERSION}" "APPLICATION_NAME=${PROJECT_NAME}" python ${python_file} ease_app.bin
                    WORKING_DIRECTORY ${build_dir}
                    VERBATIM)   