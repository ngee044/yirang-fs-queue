cmake_minimum_required(VERSION 3.21)

set(LIBRARY_NAME ThreadPool)

set(HEADER_FILES
	Job.h
	JobPool.h
	JobPriorities.h
	ThreadPool.h
	ThreadWorker.h
)

set(SOURCE_FILES
	Job.cpp
	JobPool.cpp
	JobPriorities.cpp
	ThreadPool.cpp
	ThreadWorker.cpp
)

add_library(${LIBRARY_NAME} ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${LIBRARY_NAME} PUBLIC Utilities Threads::Threads)