if(NOT DISABLE_UNITTEST)
  enable_testing()

  include_directories(${Boost_INCLUDE_DIR})

  add_executable(
    test_segment
    test_segment.cc
  )
  target_link_libraries(
    test_segment
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  )
  add_test(test_segment test_segment)

  add_executable(
    test_event_dispatcher
    test_event_dispatcher.cc
  )
  target_link_libraries(
    test_event_dispatcher
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  )
  add_test(test_event_dispatcher test_event_dispatcher)

  add_executable(
    test_entity_system
    test_entity_system.cc
  )
  target_link_libraries(
    test_entity_system
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  )
  add_test(test_entity_system test_entity_system)

endif (NOT DISABLE_UNITTEST)
