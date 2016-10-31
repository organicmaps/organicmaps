set(
  SRC
  protocol_test.cpp
  reporter_test.cpp
)

omim_add_test(tracking_tests ${SRC})
omim_link_libraries(tracking_tests tracking)
