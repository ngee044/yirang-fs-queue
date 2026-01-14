cmake_minimum_required(VERSION 3.21)

set(LIBRARY_NAME Utilities)

set(HEADER_FILES
	ArgumentParser.h
	Combiner.h
	Compressor.h
	Converter.h
	File.h
	Folder.h
	FolderWatcher.h
	Generator.h
	Log.h
	Logger.h
	LogTypes.h
)
set(SOURCE_FILES
	ArgumentParser.cpp
	Combiner.cpp
	Compressor.cpp
	Converter.cpp
	File.cpp
	Folder.cpp
	FolderWatcher.cpp
	Generator.cpp
	Log.cpp
	Logger.cpp
)

add_library(${LIBRARY_NAME} ${HEADER_FILES} ${SOURCE_FILES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${LIBRARY_NAME} PUBLIC
	efsw::efsw
	lz4::lz4
	nlohmann_json::nlohmann_json
)
