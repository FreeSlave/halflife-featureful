set (CMAKE_CXX_STANDARD 14)

cmake_minimum_required(VERSION 3.9)

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.15.0")
	cmake_policy(SET CMP0091 NEW)
endif()

project (HLSDK-FEATUREFUL-TESTS)

if(NOT MSVC)
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf )
endif()

include_directories (. ../common ../engine ../pm_shared ../game_shared ../dlls ../public ../cl_dll)

add_executable(test
	ent_templates_test.cpp
	fixed_string_test.cpp
	fixed_vector_test.cpp
	followers_test.cpp
	inventory_test.cpp
	journal_test.cpp
	materials_test.cpp
	messages_test.cpp
	objecthint_test.cpp
	parsetext_test.cpp
	soundscripts_test.cpp
	visuals_test.cpp
	warpball_test.cpp
	../game_shared/error_collector.cpp
	../game_shared/file_utils.cpp
	../game_shared/json_config.cpp
	../game_shared/json_utils.cpp
	../game_shared/parsetext.cpp
	../game_shared/random_utils.cpp
	../game_shared/tex_materials.cpp
	../game_shared/util_shared.cpp
	../public/safe_snprintf.c
	../cl_dll/inventory_hud.cpp
	../cl_dll/journal_config.cpp
	../cl_dll/message_strings.cpp
	../dlls/classify.cpp
	../dlls/ent_templates.cpp
	../dlls/followers.cpp
	../dlls/inventory.cpp
	../dlls/objecthint_spec.cpp
	../dlls/soundscripts.cpp
	../dlls/visuals.cpp
	../dlls/warpball.cpp
	main.cpp
)

target_link_libraries(test gtest)
