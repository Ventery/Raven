set(LIB_SRC
    FileTransfer.cc
)

include_directories(../Base)
include_directories(../Raven)

add_executable(raven_filetransfer ${LIB_SRC})

target_link_libraries(raven_filetransfer raven_util)
