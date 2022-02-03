set(LIB_SRC
    Util.cc
    RavenConfig.cc
    HptpContext.cc
    P2PClientBase.cc
)

add_library(raven_util ${LIB_SRC})
target_link_libraries(raven_util raven_base)
