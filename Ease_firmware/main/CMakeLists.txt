
set(srcs )

list(APPEND srcs  main.c
        # test.c
        sources/system/system.c
        sources/ble/ble.c
        sources/time/time.c
        sources/nvs/nvs.c
        sources/fuel_gauge/fuel_gauge.c
        sources/tdcs/tdcs.c
        sources/ads/ads.c
       )

set(inlude_dir)

list(APPEND include_dir
        "sources/system"
        "sources/ble"
        "sources/nvs"
        "sources/fuel_gauge"
        "sources/tdcs"
        "sources/ads"
        "sources/time"
        "." )


idf_component_register(SRCS ${srcs}

                    INCLUDE_DIRS ${include_dir})



# set_property( TARGET  APPEND_STRING  PROPERTY  <EXTRA_PARTITION_SUBTYPES> <data>,<fact_metas>,<20> )

# idf_build_set_property(EXTRA_PARTITION_SUBTYPES APPEND "meta_data,fact_meta,4")

# get_property(new_var GLOBAL PROPERTY EXTRA_PARTITION_SUBTYPES)

# message("the property is ${new_var}")