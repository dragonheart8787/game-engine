add_executable(replay_diff
  main.cpp
)

target_link_libraries(replay_diff PRIVATE nlohmann_json::nlohmann_json)
