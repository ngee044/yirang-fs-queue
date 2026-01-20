cmake_minimum_required(VERSION 3.21)

find_package(nlohmann_json CONFIG REQUIRED)

add_subdirectory(BackendAdapter)

set(PROGRAM_NAME MainMQ)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(SOURCE_FILES
	main.cpp
	Configurations.cpp
	QueueManager.cpp
	MessageValidator.cpp
	MailboxHandler.cpp
)

set(HEADER_FILES
	Configurations.h
	QueueManager.h
	MessageValidator.h
	MailboxHandler.h
	MailboxTypes.h
)

project(${PROGRAM_NAME} VERSION 1.0.0.0)

add_executable(${PROGRAM_NAME} ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(${PROGRAM_NAME} PUBLIC
	Utilities
	DataBase
	ThreadPool
	BackendAdapter
	nlohmann_json::nlohmann_json
)
target_include_directories(${PROGRAM_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

set(RESOURCE_FILES
	main_mq_configuration.json
	sqlite_schema.sql
)

foreach(RESOURCE_FILE IN LISTS RESOURCE_FILES)
	add_custom_command(
		TARGET MainMQ POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE_FILE}
			${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${RESOURCE_FILE}
	)
endforeach()
