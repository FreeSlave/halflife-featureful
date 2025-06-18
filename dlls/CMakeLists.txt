#
# Copyright (c) 2015 Pavlo Lavrenenko
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
project (SVDLL)

set (SVDLL_LIBRARY server)

add_definitions(-DSERVER_DLL)

if(NOT MSVC)
	add_compile_options(-fno-exceptions) # GCC/Clang flag
	add_compile_options(-fno-rtti) # GCC/Clang flag
	add_compile_options(-Wno-invalid-offsetof) # GCC/Clang flag
	add_compile_options(-fvisibility=hidden) # GCC/Clang flag
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_vsnprintf=vsnprintf )
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

set (SVDLL_SOURCES
	agrunt.cpp
	airtank.cpp
	aflock.cpp
	ammo_amounts.cpp
	ammoregistry.cpp
	ammunition.cpp
	animating.cpp
	animation.cpp
	apache.cpp
	barnacle.cpp
	barney.cpp
	bigmomma.cpp
	bloater.cpp
	bmodels.cpp
	bullsquid.cpp
	buttons.cpp
	cbase.cpp
	classify.cpp
	client.cpp
	combat.cpp
	common_soundscripts.cpp
	controller.cpp
	crossbow.cpp
	crowbar.cpp
	defaultai.cpp
	displacer.cpp
	displacerball.cpp
	doors.cpp
	drillsergeant.cpp
	eagle.cpp
	effects.cpp
	egon.cpp
	ent_templates.cpp
	explode.cpp
	fgrunt.cpp
	flybee.cpp
	flyingmonster.cpp
	followers.cpp
	followingmonster.cpp
	func_break.cpp
	func_tank.cpp
	game.cpp
	gamerules.cpp
	gargantua.cpp
	gauss.cpp
	genericmonster.cpp
	geneworm.cpp
	ggrenade.cpp
	globals.cpp
	glock.cpp
	gman.cpp
	gonome.cpp
	graphic_debug.cpp
	grapple.cpp
	h_ai.cpp
	h_battery.cpp
	h_cine.cpp
	h_cycler.cpp
	h_export.cpp
	handgrenade.cpp
	hassassin.cpp
	headcrab.cpp
	healthkit.cpp
	hgrunt.cpp
	hwgrunt.cpp
	hornet.cpp
	hornetgun.cpp
	houndeye.cpp
	ichthyosaur.cpp
	inventory.cpp
	islave.cpp
	items.cpp
	knife.cpp
	leech.cpp
	lights.cpp
	locus.cpp
	m249.cpp
	mapconfig.cpp
	maprules.cpp
	massn.cpp
	medkit.cpp
	monstermaker.cpp
	monsters.cpp
	monsterstate.cpp
	mortar.cpp
	mp5.cpp
	multiplay_gamerules.cpp
	nihilanth.cpp
	nodes.cpp
	nuclearbomb.cpp
	objecthint_spec.cpp
	observer.cpp
	op4mortar.cpp
	osprey.cpp
	panthereye.cpp
	pathcorner.cpp
	pipewrench.cpp
	pitdrone.cpp
	pitworm.cpp
	plane.cpp
	plats.cpp
	player.cpp
	playermonster.cpp
	python.cpp
	rat.cpp
	rgrunt.cpp
	recruit.cpp
	roach.cpp
	robocop.cpp
	ropes.cpp
	rpg.cpp
	satchel.cpp
	savetitles.cpp
	schedule.cpp
	scientist.cpp
	scripted.cpp
	shockbeam.cpp
	shockrifle.cpp
	shocktrooper.cpp
	shotgun.cpp
	singleplay_gamerules.cpp
	skill.cpp
	sniperrifle.cpp
	sound.cpp
	soundent.cpp
	soundscripts.cpp
	spectator.cpp
	spore.cpp
	sporelauncher.cpp
	squadmonster.cpp
	squeakgrenade.cpp
	subs.cpp
	talkmonster.cpp
	teamplay_gamerules.cpp
	tempmonster.cpp
	tentacle.cpp
	triggers.cpp
	tripmine.cpp
	turret.cpp
	uzi.cpp
	util.cpp
	vehicle.cpp
	visuals.cpp
	visuals_utils.cpp
	voltigore.cpp
	warpball.cpp
	weapon_templates.cpp
	weapons.cpp
	weapons_shared.cpp
	world.cpp
	xen.cpp
	zombie.cpp
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
	../game_shared/vcs_info.cpp
	../public/safe_snprintf.c
)

if(NOT MSVC)
	set_source_files_properties(ropes.cpp PROPERTIES COMPILE_FLAGS -O0)
endif()

include_directories (. wpn_shared ../common ../engine ../pm_shared ../game_shared ../public)

if(MSVC)
	set(SVDLL_SOURCES
		${SVDLL_SOURCES}
		hl.def)
endif()

if(USE_VOICEMGR)
	set(SVDLL_SOURCES
		${SVDLL_SOURCES}
		voice_gamemgr.cpp)
else()
	add_definitions(-DNO_VOICEGAMEMGR)
endif()

add_library (${SVDLL_LIBRARY} SHARED ${SVDLL_SOURCES})

set_target_properties (${SVDLL_LIBRARY} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Android" OR XASH_TERMUX)
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME}")

	set_target_properties(${SVDLL_LIBRARY} PROPERTIES
		OUTPUT_NAME "${SVDLL_NAME}${POSTFIX}"
		PREFIX "")
endif()

if(MSVC)
	set_property(TARGET ${SVDLL_LIBRARY} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(HAVE_LTO)
	set_property(TARGET ${SVDLL_LIBRARY} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install( TARGETS ${SVDLL_LIBRARY}
	RUNTIME DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
	LIBRARY DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	    GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE)

if(CMAKE_BUILD_TYPE MATCHES "Release")
	add_custom_command(TARGET ${SVDLL_LIBRARY}
			POST_BUILD DEPENDS ${SVDLL_LIBRARY}
			COMMAND ${CMAKE_STRIP} -s $<TARGET_FILE:${SVDLL_LIBRARY}>)
endif()
