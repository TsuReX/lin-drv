cmake_minimum_required(VERSION 2.6)

# Тип сборки по умолчанию - релиз.
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# Настраиваем уникальный идентификатор версии проекта в Mercurial.
set(MERCURIAL_LONG_CHANGESET "0x0" CACHE STRING "Long changeset number.")
set(MERCURIAL_SHORT_CHANGESET "0" CACHE STRING "Short changeset number.")

# Настраиваем пути к общим заголовочным файлам.
set(EXPORT_HEADERS_DIR ${CMAKE_SOURCE_DIR}/export CACHE INTERNAL "Export header files location (/CMAKE_SOURCE_DIR/export)." FORCE)
set(COMMON_HEADERS_DIR ${CMAKE_SOURCE_DIR}/include CACHE INTERNAL "Common header files location (/CMAKE_SOURCE_DIR/include)." FORCE)

# Configure a header file to pass some of the CMake settings to the source code.
configure_file("${COMMON_HEADERS_DIR}/mvs_version.h.in" "${CMAKE_BINARY_DIR}/mvs_version.h" @ONLY NEWLINE_STYLE UNIX)

# Подключаем подпроекты.
add_subdirectory(library)
add_subdirectory(driver)

