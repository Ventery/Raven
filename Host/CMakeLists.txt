set(LIB_SRC
    P2PHost.cc
)

include_directories(../Base)
include_directories(../Raven)

add_library(raven_host ${LIB_SRC})

target_link_libraries(raven_host raven_util)

add_subdirectory(Test)


