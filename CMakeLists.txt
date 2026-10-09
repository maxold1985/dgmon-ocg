cmake_minimum_required(VERSION 3.10)
project(digimon_hyper_colosseum_2002 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Recover the offline 283-card catalog shipped as a compressed source asset.
# Avoid network downloads and keep the C++11 project self-contained.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/cards.csv")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar xzf "${CMAKE_CURRENT_SOURCE_DIR}/data/cards.tar.gz"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE extract_result)
  if(NOT extract_result EQUAL 0)
    message(FATAL_ERROR "Could not extract data/cards.csv from data/cards.tar.gz")
  endif()
endif()

add_library(hc_core STATIC src/engine.cpp src/card_catalog.cpp)
target_include_directories(hc_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_executable(hc_cards src/main.cpp)
target_link_libraries(hc_cards hc_core)
enable_testing()
add_test(NAME catalog_and_battle COMMAND hc_cards "${CMAKE_CURRENT_SOURCE_DIR}/data/cards.csv" --demo)
add_test(NAME booster_card COMMAND hc_cards "${CMAKE_CURRENT_SOURCE_DIR}/data/cards.csv" --card Bo-1)
add_executable(hc_core_tests tests/core_tests.cpp)
target_link_libraries(hc_core_tests hc_core)
add_test(NAME full_core_tests COMMAND hc_core_tests "${CMAKE_CURRENT_SOURCE_DIR}/data/cards.csv")
