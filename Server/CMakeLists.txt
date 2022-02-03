set(LIB_SRC
    P2PServer.cc
)

include_directories(../Base)
include_directories(../Raven)

add_library(raven_server ${LIB_SRC})

target_link_libraries(raven_server raven_util)

add_subdirectory(Test)


