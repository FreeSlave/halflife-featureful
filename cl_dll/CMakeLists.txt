#
# Copyright (c) 2016 Alibek Omarov
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 3.9)
project (CLDLL)

set (CLDLL_LIBRARY client)

add_definitions(-DCLIENT_DLL)

if(NOT MSVC)
	add_compile_options(-fno-exceptions) # GCC/Clang flag
	add_compile_options(-fno-rtti) # GCC/Clang flag
	add_compile_options(-fvisibility=hidden) # GCC/Clang flag
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp)
	if(NOT MINGW)
		add_definitions(-D_vsnprintf=vsnprintf)
	endif()
	if(CMAKE_BUILD_TYPE MATCHES "Release"
		OR (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo"
		AND CMAKE_SYSTEM_NAME STREQUAL "Android"))
		add_compile_options(-fno-unwind-tables -fno-asynchronous-unwind-tables) # GCC/Clang flag
		add_compile_options(-fomit-frame-pointer) # GCC/Clang flag
		add_compile_options(-ftree-vectorize) # GCC/Clang flag
		add_compile_options(-funsafe-math-optimizations) # GCC/Clang flag
	endif()
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

if (GOLDSOURCE_SUPPORT)
	add_definitions(-DGOLDSOURCE_SUPPORT)
endif()

if (USE_VGUI)
	add_definitions(-DUSE_VGUI)
endif()

set (CLDLL_SOURCES
	../dlls/ammoregistry.cpp
	../dlls/weapons_shared.cpp
	../dlls/crossbow.cpp
	../dlls/crowbar.cpp
	../dlls/egon.cpp
	../dlls/gauss.cpp
	../dlls/handgrenade.cpp
	../dlls/hornetgun.cpp
	../dlls/knife.cpp
	../dlls/mp5.cpp
	../dlls/python.cpp
	../dlls/rpg.cpp
	../dlls/satchel.cpp
	../dlls/shotgun.cpp
	../dlls/squeakgrenade.cpp
	../dlls/tripmine.cpp
	../dlls/glock.cpp
	../dlls/eagle.cpp
	../dlls/pipewrench.cpp
	../dlls/grapple.cpp
	../dlls/sniperrifle.cpp
	../dlls/m249.cpp
	../dlls/displacer.cpp
	../dlls/shockrifle.cpp
	../dlls/sporelauncher.cpp
	../dlls/medkit.cpp
	../dlls/uzi.cpp
	ev_hldm.cpp
	hl/hl_baseentity.cpp
	hl/hl_events.cpp
	hl/hl_objects.cpp
	hl/hl_weapons.cpp
	ammo.cpp
	ammo_secondary.cpp
	ammohistory.cpp
	cdll_int.cpp
	cl_fx.cpp
	cl_msg_fx.cpp
	com_weapons.cpp
	death.cpp
	demo.cpp
	entity.cpp
	environment.cpp
	ev_common.cpp
	events.cpp
	flashlight.cpp
	GameStudioModelRenderer.cpp
	geiger.cpp
	health.cpp
	hud.cpp
	hud_caption.cpp
	hud_error_collection.cpp
	hud_journal.cpp
	hud_monsterinfo.cpp
	hud_msg.cpp
	hud_redraw.cpp
	hud_spectator.cpp
	hud_speedometer.cpp
	hud_renderer.cpp
	hud_update.cpp
	in_camera.cpp
	input.cpp
	input_goldsource.cpp
	input_mouse.cpp
	input_xash3d.cpp
	interpolation.cpp
	inventory_hud.cpp
	journal_config.cpp
	menu.cpp
	message.cpp
	message_strings.cpp
	MOTD.cpp
	nightvision.cpp
	objecthint_manager.cpp
	parsemsg.cpp
	../pm_shared/pm_debug.cpp
	../pm_shared/pm_math.cpp
	../pm_shared/pm_shared.cpp
	../game_shared/tex_materials.cpp
	../game_shared/parsetext.cpp
	../game_shared/error_collector.cpp
	../game_shared/file_utils.cpp
	../game_shared/fx_types.cpp
	../game_shared/json_config.cpp
	../game_shared/json_utils.cpp
	../game_shared/random_utils.cpp
	../game_shared/unicode.cpp
	../game_shared/util_shared.cpp
	saytext.cpp
	scoreboard.cpp
	status_icons.cpp
	statusbar.cpp
	studio_util.cpp
	StudioModelRenderer.cpp
	text_message.cpp
	train.cpp
	tri.cpp
	util.cpp
	view.cpp
	../game_shared/vcs_info.cpp
	particleman/CBaseParticle.cpp
	particleman/CFrustum.cpp
	particleman/CMiniMem.cpp
	particleman/IParticleMan_Active.cpp
	../public/safe_snprintf.c
)

if (USE_VGUI)
	list(APPEND CLDLL_SOURCES
		vgui_int.cpp
		vgui_ClassMenu.cpp
		vgui_ConsolePanel.cpp
		vgui_ControlConfigPanel.cpp
		vgui_CustomObjects.cpp
		vgui_MOTDWindow.cpp
		vgui_SchemeManager.cpp
		vgui_ScorePanel.cpp
		vgui_TeamFortressViewport.cpp
		vgui_SpectatorPanel.cpp
		vgui_teammenu.cpp
		voice_status.cpp
		vgui_checkbutton2.cpp
		vgui_grid.cpp
		vgui_helpers.cpp
		vgui_listbox.cpp
		vgui_loadtga.cpp
		vgui_scrollbar2.cpp
		vgui_slider2.cpp
		voice_banmgr.cpp
		)
endif()

include_directories (. hl/ ../dlls ../dlls/wpn_shared ../common ../engine ../pm_shared ../game_shared ../public particleman/)

if (WIN32)
	include_directories (../external/)
endif()

if (USE_VGUI)
	SET(CMAKE_SKIP_RPATH TRUE)
	link_directories(${CMAKE_SOURCE_DIR}/vgui_support/vgui-dev/lib)
	include_directories(../vgui_support/vgui-dev/include)
else()
	include_directories(../utils/fake_vgui/include)
endif()

if(USE_VOICEMGR)
	#set(CLDLL_SOURCES
	#	${CLDLL_SOURCES}
	#	../game_shared/voice_banmgr.cpp
	#	../game_shared/voice_status.cpp)
endif()

add_library (${CLDLL_LIBRARY} SHARED ${CLDLL_SOURCES})

if (GOLDSOURCE_SUPPORT)
	target_link_libraries( ${CLDLL_LIBRARY} ${CMAKE_DL_LIBS} )
endif()

if (USE_VGUI)
	if (WIN32)
		add_library(vgui SHARED IMPORTED)
		set_property(TARGET vgui PROPERTY IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vgui_support/vgui-dev/lib/win32_vc6/vgui.dll")
		set_property(TARGET vgui PROPERTY IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/vgui_support/vgui-dev/lib/win32_vc6/vgui.lib")
		target_link_libraries(${CLDLL_LIBRARY} vgui)
	elseif(APPLE)
		target_link_libraries(${CLDLL_LIBRARY} "-Wl,--no-undefined -L${CMAKE_SOURCE_DIR}/vgui_support/vgui-dev/lib vgui.dylib")
	else()
		target_link_libraries(${CLDLL_LIBRARY} :vgui.so)
	endif()
endif()

if(WIN32)
	target_link_libraries( ${CLDLL_LIBRARY} user32.lib )
	if (GOLDSOURCE_SUPPORT)
		target_link_libraries( ${CLDLL_LIBRARY} winmm.lib )
	endif()
endif()

set_target_properties (${CLDLL_LIBRARY} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Android" OR XASH_TERMUX)
	set(CLDLL_NAME "client")
	set_target_properties(${CLDLL_LIBRARY} PROPERTIES
		OUTPUT_NAME "${CLDLL_NAME}${POSTFIX}"
		PREFIX "")
endif()

if(MSVC)
	set_property(TARGET ${CLDLL_LIBRARY} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(HAVE_LTO)
	set_property(TARGET ${CLDLL_LIBRARY} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install( TARGETS ${CLDLL_LIBRARY}
	RUNTIME DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
	LIBRARY DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	    GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE )

if(CMAKE_BUILD_TYPE MATCHES "Release")
	add_custom_command(TARGET ${CLDLL_LIBRARY}
			POST_BUILD DEPENDS ${CLDLL_LIBRARY}
			COMMAND ${CMAKE_STRIP} -s $<TARGET_FILE:${CLDLL_LIBRARY}>)
endif()
