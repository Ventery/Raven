set(LIB_SRC
    FileTransfer.cc
)

include_directories(../Base)
include_directories(../Raven)

add_executable(TestFileTransfer ${LIB_SRC})

target_link_libraries(TestFileTransfer raven_util)
