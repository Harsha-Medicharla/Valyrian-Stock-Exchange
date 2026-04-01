cmake_minimum_required(VERSION 3.10)
project(ValyrianSettlement)

# 1. Find the PostgreSQL package on the system
# This sets variables like PostgreSQL_INCLUDE_DIRS and PostgreSQL_LIBRARIES
find_package(PostgreSQL REQUIRED)

# 2. Define the source files
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

# 3. Create the library
add_library(settlement_lib STATIC ${SETTLEMENT_SOURCES})

# 4. Tell the compiler where to find YOUR headers AND PostgreSQL headers
target_include_directories(settlement_lib 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE 
        ${PostgreSQL_INCLUDE_DIRS}
)

# 5. Link the necessary libraries
target_link_libraries(settlement_lib 
    PRIVATE 
        Threads::Threads
        ${PostgreSQL_LIBRARIES}
)