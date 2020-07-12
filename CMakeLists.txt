string(REPLACE "\\" "/" IDF_PART_NORM $ENV{IDF_PATH})

idf_component_register(
    SRCS 
        "lib/Trill.cpp"
    INCLUDE_DIRS
        "${IDF_PART_NORM}/components/driver/include"
)

target_compile_options(${COMPONENT_LIB} PUBLIC -DUSE_ESP32)


