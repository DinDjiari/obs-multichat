cmake_minimum_required(VERSION 3.16)

project(multichat VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# --- Qt6 -------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network WebSockets)

# --- libobs and frontend API via pkg-config / manual lookup ----------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBOBS REQUIRED libobs)
pkg_check_modules(LIBSECRET REQUIRED libsecret-1)

# The frontend API does not always ship a .pc file, so locate it manually.
find_path(OBS_FRONTEND_INCLUDE_DIR
	NAMES obs-frontend-api.h
	PATHS
		${LIBOBS_INCLUDE_DIRS}
		/usr/include/obs
		/usr/include
	PATH_SUFFIXES obs
)

find_library(OBS_FRONTEND_LIB
	NAMES obs-frontend-api
	PATHS
		${LIBOBS_LIBRARY_DIRS}
		/usr/lib
		/usr/lib/obs-plugins
)

if(NOT OBS_FRONTEND_INCLUDE_DIR OR NOT OBS_FRONTEND_LIB)
	message(FATAL_ERROR
		"obs-frontend-api not found. Install the OBS Studio development files.")
endif()

# --- Sources ---------------------------------------------------------------
# Headers carrying Q_OBJECT are listed so AUTOMOC processes them.
set(multichat_SOURCES
	src/plugin-main.cpp
	src/config-manager.cpp
	src/token-store.cpp
	src/device-flow-auth.cpp
	src/twitch-client.cpp
	src/youtube-client.cpp
	src/chat-tab.cpp
	src/alerts-tab.cpp
	src/settings-tab.cpp
	src/multichat-dock.cpp
)

set(multichat_HEADERS
	src/chat-types.hpp
	src/html-util.hpp
	src/config-manager.hpp
	src/token-store.hpp
	src/device-flow-auth.hpp
	src/twitch-client.hpp
	src/youtube-client.hpp
	src/chat-tab.hpp
	src/alerts-tab.hpp
	src/settings-tab.hpp
	src/multichat-dock.hpp
)

add_library(multichat MODULE ${multichat_SOURCES} ${multichat_HEADERS})

# OBS expects the module file to be named "multichat.so" (no "lib" prefix).
set_target_properties(multichat PROPERTIES PREFIX "")

target_include_directories(multichat PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${LIBOBS_INCLUDE_DIRS}
	${OBS_FRONTEND_INCLUDE_DIR}
	${LIBSECRET_INCLUDE_DIRS}
)

target_link_libraries(multichat PRIVATE
	Qt6::Core
	Qt6::Widgets
	Qt6::Network
	Qt6::WebSockets
	${LIBOBS_LIBRARIES}
	${OBS_FRONTEND_LIB}
	${LIBSECRET_LIBRARIES}
)

target_compile_options(multichat PRIVATE ${LIBOBS_CFLAGS_OTHER} ${LIBSECRET_CFLAGS_OTHER})

# --- Install ---------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS multichat
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/obs-plugins
)

install(DIRECTORY data/
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/obs/obs-plugins/multichat
)
