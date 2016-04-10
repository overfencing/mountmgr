set(TARGET tmnt)

set(SRCS
    main.c
)

add_executable(${TARGET} ${SRCS})
target_link_libraries(${TARGET} c tosmnt)

install(
    TARGETS ${TARGET}
    DESTINATION bin
)

