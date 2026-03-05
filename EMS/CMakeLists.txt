add_library(ems STATIC
    core/EMS.cpp
    dispatcher/Dispatcher.cpp
    pipeline/EMSPipeline.cpp
    threading/IngressWorker.cpp
    auth/AuthService.cpp
    risk/RiskManager.cpp
    market/MarketState.cpp
    routing/SymbolRouter.cpp
    tracker/EMSOrderTracker.cpp
    ports/RingBufferEgress.cpp  # <-- JUST ADD THIS LINE
)

target_compile_features(ems PUBLIC cxx_std_17)

# THIS IS THE IMPORTANT PART
target_include_directories(ems
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ems
    PUBLIC
        absl::flat_hash_map
        art
        wal
)