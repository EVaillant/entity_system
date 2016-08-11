if(NOT DISABLE_DEMOS)

    include_directories(
      ${ALLEGRO5_INCLUDE_DIRS}
    )

    add_executable(
      snake
      snake.cc
    )
    target_link_libraries(
      snake
      ${ALLEGRO5_LDFLAGS}
    )

endif (NOT DISABLE_DEMOS)
