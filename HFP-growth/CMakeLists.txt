include_directories(${Boost_INCLUDE_DIRS})
include_directories (${CMAKE_SOURCE_DIR}/common)

add_executable (HFP-growth main.cpp HFPGrowth.cpp FPTree.cpp) 
target_link_libraries(HFP-growth stdc++fs gimlet ${Boost_LIBRARIES} boost_program_options)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/HFP-growth
  DESTINATION bin)
