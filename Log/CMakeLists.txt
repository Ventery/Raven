set(LIB_SRC
    AsyncLogging.cc
    CountDownLatch.cc
    FileUtil.cc
    LogFile.cc
    Logging.cc
    LogStream.cc
    Thread.cc
)

add_library(raven_log ${LIB_SRC})
target_link_libraries(raven_log pthread rt)

#set_target_properties(libserver_base PROPERTIES OUTPUT_NAME "server_base")
