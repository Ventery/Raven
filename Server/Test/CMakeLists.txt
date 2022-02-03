set(SRCS
    TestServer.cc
)
include_directories(../ )
add_executable(TestRavenServer ${SRCS})
target_link_libraries(TestRavenServer raven_server)