cmake_minimum_required(VERSION 3.10)
project(ValyrianSettlement)

set(SETTLEMENT_SOURCES
    core/Settlement.cpp
    core/TradeValidator.cpp
    core/PartialFillHandler.cpp
    funds/FundManager.cpp
    funds/ShareManager.cpp
    market/SymbolDataUpdater.cpp
    confirmation/ConfirmationGenerator.cpp
    persistence/DBSyncWorker.cpp
    persistence/PostgresWriter.cpp
)

# This name MUST match what you use in target_link_libraries in the root!
add_library(settlement_lib STATIC ${SETTLEMENT_SOURCES})

target_include_directories(settlement_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(settlement_lib 
    PRIVATE 
        Threads::Threads
        ${PQXX_LIBRARIES}
        pq
)