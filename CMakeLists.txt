#
# CMake file to build ReverseShell.
# Mark Grimes (mark.grimes@rymapt.com)
# 14/Nov/2017
# copyright 2017 Rymapt Ltd
# Licence to be decided
#

project( ReverseShell )

cmake_minimum_required(VERSION 2.8)

if( NOT MSVC ) # Microsoft Visual Studio is C++11 by default and doesn't recognise this flag
	add_definitions( "-std=c++11" )
endif()


# Create the file that has the version information and git hash
execute_process( COMMAND git describe --dirty --always --tags WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE ReverseShell_GIT_DESCRIBE OUTPUT_STRIP_TRAILING_WHITESPACE )
execute_process( COMMAND git log -1 --pretty=format:%H WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE ReverseShell_GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE )
add_custom_target( CreateVersionFile ALL ${CMAKE_COMMAND} -E touch "${CMAKE_SOURCE_DIR}/src/version.cpp.in" ) # Make sure the git hash is always checked (not just at configure time)
configure_file( "${CMAKE_SOURCE_DIR}/src/version.cpp.in" "${CMAKE_BINARY_DIR}/src/version.cpp" )

#
# Dependencies
# TODO - allow overriding of the bundled versions with system versions
#
include_directories( ${CMAKE_SOURCE_DIR}/external/asio/include )
# tell websocketpp to use asio standalone, and full C++11 support instead of boost stand-ins
add_definitions( "-DASIO_STANDALONE -D_WEBSOCKETPP_CPP11_STRICT_ -D_WEBSOCKETPP_CPP11_STL_" )
include_directories( ${CMAKE_SOURCE_DIR}/external/websocketpp )
include_directories( ${CMAKE_SOURCE_DIR}/external/pstreams )
include_directories( ${CMAKE_SOURCE_DIR}/external/clara )
#
# Find OpenSSL
#
find_package( OpenSSL REQUIRED )
include_directories( ${OPENSSL_INCLUDE_DIR} )


include_directories( "${CMAKE_SOURCE_DIR}/include" )
aux_source_directory( "${CMAKE_SOURCE_DIR}/src" library_sources )
list( REMOVE_ITEM library_sources "${CMAKE_SOURCE_DIR}/src/main_client.cpp" ) # belongs only in the executable
list( REMOVE_ITEM library_sources "${CMAKE_SOURCE_DIR}/src/main_server.cpp" ) # belongs only in the executable

add_library( ${PROJECT_NAME}_LIB STATIC ${library_sources} "${CMAKE_BINARY_DIR}/src/version.cpp" )
target_link_libraries( ${PROJECT_NAME}_LIB ${OPENSSL_LIBRARIES} )
if( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
	# For some reason linux gets "undefined reference to `pthread_create'" even though
	# the code uses std::thread.
	target_link_libraries( ${PROJECT_NAME}_LIB "pthread" )
endif()
add_executable( ${PROJECT_NAME}Client "${CMAKE_SOURCE_DIR}/src/main_client.cpp" )
target_link_libraries( ${PROJECT_NAME}Client ${PROJECT_NAME}_LIB )
add_executable( ${PROJECT_NAME}Server "${CMAKE_SOURCE_DIR}/src/main_server.cpp" )
target_link_libraries( ${PROJECT_NAME}Server ${PROJECT_NAME}_LIB )

install( TARGETS ${PROJECT_NAME}_LIB DESTINATION lib )
install( DIRECTORY "${CMAKE_SOURCE_DIR}/include/" DESTINATION include )

option( BUILD_UNITTESTS "Build unit tests" ON )
message( STATUS "BUILD_UNITTESTS: ${BUILD_UNITTESTS}" )
if( BUILD_UNITTESTS )
	# Fix the test configuration file to have the correct paths
	configure_file( "${PROJECT_SOURCE_DIR}/tests/src/testinputs.cpp.in" "${PROJECT_BINARY_DIR}/tests/src/testinputs.cpp" @ONLY )
	include_directories( "${CMAKE_SOURCE_DIR}/tests/include" )
	aux_source_directory( "${CMAKE_SOURCE_DIR}/tests/src" unittests_sources )
	add_executable( ${PROJECT_NAME}Tests ${unittests_sources} "${PROJECT_BINARY_DIR}/tests/src/testinputs.cpp" )
	target_link_libraries( ${PROJECT_NAME}Tests ${PROJECT_NAME}_LIB )
endif()

#
# Add a target to create a docker container for the server
#
if( "${CMAKE_SYSTEM}" MATCHES "Linux" )
	find_program( DOCKER_COMMAND docker )
	if( NOT ${DOCKER_COMMAND} MATCHES "NOTFOUND" )
		# Copy the required system libraries to a separate directory
		add_custom_command( OUTPUT "${CMAKE_BINARY_DIR}/dockerSystemLibs"
			COMMAND ${CMAKE_SOURCE_DIR}/scripts/extractDynamicLibs.sh ${PROJECT_NAME}Server "${CMAKE_BINARY_DIR}/dockerSystemLibs" )
		# Copy the Dockerfile because docker insists it's in the build context
		add_custom_command( OUTPUT "${CMAKE_BINARY_DIR}/Dockerfile"
			COMMAND cp "${CMAKE_SOURCE_DIR}/Dockerfile" "${CMAKE_BINARY_DIR}/Dockerfile"
			DEPENDS "${CMAKE_SOURCE_DIR}/Dockerfile" )
		# Create the image and use the short git hash as the tag
		add_custom_target( docker_container
			COMMAND docker build -t reverseshell-server:`git show --pretty=format:%h -s` -t reverseshell-server:latest "${CMAKE_BINARY_DIR}"
			DEPENDS "${CMAKE_BINARY_DIR}/Dockerfile" "${CMAKE_BINARY_DIR}/dockerSystemLibs" ${PROJECT_NAME}Server )
	else()
		message( STATUS "Not adding the target \"docker_container\" because the docker command was not found" )
	endif()
else()
	message( STATUS "Not adding the target \"docker_container\" because this isn't a Linux build" )
endif()
