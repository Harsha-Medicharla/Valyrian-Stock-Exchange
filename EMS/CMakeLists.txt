cmake_minimum_required(VERSION 3.14)

# Define the EMS library
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
    ports/RingBufferEgress.cpp 
)

# Use C++17
target_compile_features(ems PUBLIC cxx_std_17)

# ---------------------------------------------------------
# Include Directories
# ---------------------------------------------------------
target_include_directories(ems
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
)

# ---------------------------------------------------------
# Link Libraries
# ---------------------------------------------------------
target_link_libraries(ems
    PUBLIC
        absl::flat_hash_map
        art
        wal
        settlement_lib   # FIXED: Changed from 'settlement'
        Threads::Threads
)