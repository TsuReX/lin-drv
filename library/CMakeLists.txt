cmake_minimum_required(VERSION 2.6)

project(HardwareInterface)

# Задаем списки исходных текстов и заголовочных файлов.
set(HEADERS ${EXPORT_HEADERS_DIR}/HardwareInterface.h
            ${CMAKE_BINARY_DIR}/mvs_version.h
            ${COMMON_HEADERS_DIR}/mvs_types.h
            ${COMMON_HEADERS_DIR}/mvs_types_linux.h
            ${COMMON_HEADERS_DIR}/mvs_types_windows.h
            ${CMAKE_CURRENT_SOURCE_DIR}/internals.h
)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.c
            ${CMAKE_CURRENT_SOURCE_DIR}/arch_linux.c
            ${CMAKE_CURRENT_SOURCE_DIR}/arch_windows.c
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
                    ${EXPORT_HEADERS_DIR}
                    ${COMMON_HEADERS_DIR}
)

if(MSVC)
	# Заполняем группы для корректного отображения в MS Visual C++.
	source_group("Header Files" FILES ${HEADERS})
	source_group("Source Files" FILES ${SOURCES})

	# Disable deprecation warnings.
	add_definitions(-D_POSIX_ -D_CRT_SECURE_NO_DEPRECATE)

	# Add a module-definition (.def) file.
	if(NOT CMAKE_CL_64)
		set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/msvc_exports.def)
	endif()
endif()

add_library(${PROJECT_NAME} MODULE ${SOURCES})

if(MSVC)
	# Выводим все возможные предупреждения при компиляции, используем статическую компоновку.
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/W4 /MT")
else()
	# Выводим все возможные предупреждения при компиляции, отключаем strict aliasing.
	set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-O3 -Wall -fno-strict-aliasing")

	# Подключаем дополнительные библиотеки.
	target_link_libraries(${PROJECT_NAME} rt)
endif()

# Set the name of the preprocessor symbol defined when compiling sources in a shared library.
set_target_properties(${PROJECT_NAME} PROPERTIES DEFINE_SYMBOL HI_LIBRARY)

# Убираем префикс имени файла библиотеки.
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

