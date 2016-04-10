set(TARGET tosmnt)

set(SRCS
    conn.c
    mount.c
    query.c
)

add_library(${TARGET} SHARED ${SRCS} $<TARGET_OBJECTS:mountmgr_common>)
target_link_libraries(${TARGET} jansson)

set_target_properties(
    ${TARGET}
    PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)

install(
    TARGETS ${TARGET}
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

