idf_component_register(
    SRCS 
        "main.c"
        "../../benchmark.c"
        "../../../src/fastcron.c"
        "../../third_party/ccronexpr/ccronexpr.c"
    INCLUDE_DIRS 
        "."
        "../../../include"
        "../../third_party/ccronexpr"
)
target_compile_definitions(${COMPONENT_LIB} PUBLIC ESP_PLATFORM)
