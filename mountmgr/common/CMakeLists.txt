set(TARGET mountmgr_common)

set(SRCS
    io.c
)

add_library(${TARGET} OBJECT ${SRCS})

