add_executable(render_contract_tests
  main.cpp
)

target_link_libraries(render_contract_tests PRIVATE engine_render)

add_test(NAME render_contract_tests COMMAND render_contract_tests)
