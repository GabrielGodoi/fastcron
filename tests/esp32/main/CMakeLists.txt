idf_component_register(
    SRCS "test_main.c" "../../test_fastcron.c" "../../test_fastcron_scheduler.c"
    INCLUDE_DIRS "."
    REQUIRES fastcron unity
)
