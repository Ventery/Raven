set(LIB_SRC
    Global.cc
    ReadConfig.cc
    ClientBase.cc
)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
set(LIB_SRC ${LIB_SRC} ServerBase.cc)
ENDIF()

add_library(raven_base ${LIB_SRC})
target_link_libraries(raven_base gcrypt pthread)


