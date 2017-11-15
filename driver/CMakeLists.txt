cmake_minimum_required(VERSION 2.6)

if(UNIX)
	project(MVS)

	# Настраиваем пути к исходным текстам сборки ядра.
	set(KERNEL_DIR /lib/modules/${CMAKE_SYSTEM_VERSION}/build CACHE PATH "A path to the kernel source top-level directory (/lib/modules/CMAKE_SYSTEM_VERSION/build).")

	# Configure a Kbuild file.
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Kbuild.in" "${CMAKE_CURRENT_BINARY_DIR}/Kbuild" @ONLY NEWLINE_STYLE UNIX)

	# Задаем списки исходных текстов.
	set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/MVS.c)

	# A Makefile is only responsible for building objects in its own directory.
	foreach(_file ${SOURCES})
		configure_file(${_file} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
	endforeach()

	add_custom_target(${PROJECT_NAME} ALL make -C ${KERNEL_DIR} M=${CMAKE_CURRENT_BINARY_DIR} modules
	                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	                  VERBATIM
	)

endif(UNIX)

