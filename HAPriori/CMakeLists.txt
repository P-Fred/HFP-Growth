include_directories(${Boost_INCLUDE_DIRS})
include_directories (${CMAKE_SOURCE_DIR}/common)

add_executable (HApriori HApriori.cpp) 
target_link_libraries(HApriori stdc++fs gimlet ${Boost_LIBRARIES} boost_program_options)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/HApriori
  DESTINATION bin)
