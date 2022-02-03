set(SRCS
    TestClient.cc
)
include_directories(../ )
add_executable(TestRavenClient ${SRCS})
target_link_libraries(TestRavenClient raven_client)