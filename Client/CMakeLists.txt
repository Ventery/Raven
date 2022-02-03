set(LIB_SRC
    P2PClient.cc
)

include_directories(../Base)
include_directories(../Raven)

add_library(raven_client ${LIB_SRC})

target_link_libraries(raven_client raven_util)

add_subdirectory(Test)

