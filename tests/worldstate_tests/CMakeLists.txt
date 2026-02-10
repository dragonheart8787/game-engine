add_executable(worldstate_tests
  main.cpp
)

target_link_libraries(worldstate_tests PRIVATE engine_world)

add_test(NAME worldstate_tests COMMAND worldstate_tests)
