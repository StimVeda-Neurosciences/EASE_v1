
# get all the files from the dfu and dfu/libs dir

file(GLOB src_files ./*.c
                        libs/*/*.c)


set(inc_dir )

foreach(file ${src_files})

get_filename_component(dir ${file} DIRECTORY)
list(APPEND inc_dir ${dir})

endforeach()

list(REMOVE_DUPLICATES inc_dir)


idf_component_register(SRCS ${src_files}
                    INCLUDE_DIRS ${inc_dir})

target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-format)

target_link_libraries(${COMPONENT_LIB} "-u app_custom_desc")