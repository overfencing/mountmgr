set(TARGET tamd)

set(SRCS
    conf.c
    drive.c
    drive_imp.c
    drivemgr.c
    event.c
    event_ipc.c
    event_mount.c
    event_unmount.c
    listener.c
    log.c
    main.c
    worker.c
)

add_executable(${TARGET} ${SRCS} $<TARGET_OBJECTS:mountmgr_common>)
target_link_libraries(${TARGET} c -pthread jansson)

install(
    TARGETS ${TARGET}
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_EXECUTE
)

