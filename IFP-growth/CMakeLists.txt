include_directories(${Boost_INCLUDE_DIRS})
include_directories (${CMAKE_SOURCE_DIR}/common)

add_executable (IFP-growth main.cpp IFPGrowth.cpp FPTree.cpp) 
target_link_libraries(IFP-growth stdc++fs gimlet ${Boost_LIBRARIES} boost_program_options ${CMAKE_THREAD_LIBS_INIT})

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/IFP-growth
  DESTINATION bin)
