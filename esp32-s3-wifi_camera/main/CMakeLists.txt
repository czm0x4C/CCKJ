idf_component_register( SRCS "main.c" "./src/wifiConnect.c" "./src/httpServer.c" "./src/camera.c" "./src/led.c" "./src/uart.c" "./src/ringBuffer.c"
                        INCLUDE_DIRS "." "./inc"
                        EMBED_FILES "./html/index.html"
                    )
