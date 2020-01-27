idf_component_register(SRCS "AppMain.c" "../mg/mongoose.c"
                    INCLUDE_DIRS "." "../mg")
spiffs_create_partition_image(storage ../dist FLASH_IN_PROJECT)