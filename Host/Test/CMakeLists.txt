set(SRCS
    TestHost.cc
)
include_directories(../ )
add_executable(TestRavenHost ${SRCS})
target_link_libraries(TestRavenHost raven_host)