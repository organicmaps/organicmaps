project(world_feed_integration_tests)

set(SRC world_feed_integration_tests.cpp)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    world_feed
    generator
)

# This test requires additional data from the other repository
if(NOT TEST_DATA_REPO_URL)
	set(TEST_DATA_REPO_URL "https://github.com/organicmaps/world_feed_integration_tests_data.git")
endif()
set(DESTINATION_FOLDER "${OMIM_DATA_DIR}/test_data/world_feed_integration_tests_data/")

include(ExternalProject)
ExternalProject_Add(world_feed_integration_tests_data 
    GIT_REPOSITORY "${TEST_DATA_REPO_URL}"
    GIT_TAG 30ecb0b3fe694a582edfacc2a7425b6f01f9fec6
    SOURCE_DIR "${DESTINATION_FOLDER}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_DISCONNECTED ON
)
add_dependencies(${PROJECT_NAME} world_feed_integration_tests_data)
