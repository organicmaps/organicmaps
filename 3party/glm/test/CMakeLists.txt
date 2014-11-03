function(glmCreateTestGTC NAME)
	if(GLM_TEST_ENABLE)
		set(SAMPLE_NAME test-${NAME})
		add_executable(${SAMPLE_NAME} ${NAME}.cpp)

		add_test(
		    NAME ${SAMPLE_NAME}
		    COMMAND $<TARGET_FILE:${SAMPLE_NAME}> )
	endif(GLM_TEST_ENABLE)
endfunction()

add_subdirectory(bug)
add_subdirectory(core)
add_subdirectory(gtc)
add_subdirectory(gtx)


