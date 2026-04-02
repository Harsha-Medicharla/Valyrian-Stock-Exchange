cmake_minimum_required(VERSION 3.10)
project(ValyrianSettlement)

# 1. Find required packages
find_package(PostgreSQL REQUIRED)
find_package(Threads REQUIRED)

# 2. Define source files
set(SETTLEMENT_SOURCES
    core/Settlement.cpp
    core/TradeValidator.cpp
    core/PartialFillHandler.cpp
    funds/FundManager.cpp
    funds/ShareManager.cpp
    market/SymbolDataUpdater.cpp
    confirmation/ConfirmationGenerator.cpp
    persistence/WAL.cpp                # 🔥 ADDED
    persistence/DBSyncWorker.cpp
    persistence/PostgresWriter.cpp
)

# 3. Create the library
add_library(settlement_lib STATIC ${SETTLEMENT_SOURCES})

# 4. Include directories
target_include_directories(settlement_lib 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE 
        ${PostgreSQL_INCLUDE_DIRS}
)

# 5. Link libraries
target_link_libraries(settlement_lib 
    PRIVATE 
        Threads::Threads
        ${PostgreSQL_LIBRARIES}
)