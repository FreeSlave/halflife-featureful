@echo off
echo Setting environment for minimal Visual C++ 6
set INCLUDE=%MSVCDir%\include
set LIB=%MSVCDir%\lib
set PATH=%MSVCDir%\bin;%PATH%

echo -- Compiler is MSVC6

set XASH3DSRC=..\..\Xash3D_original
set INCLUDES=-I../common -I../engine -I../pm_shared -I../game_shared -I../public -I../external -I../dlls -I../utils/fake_vgui/include
set SOURCES=../dlls/weapons_shared.cpp ^
	../dlls/crossbow.cpp ^
	../dlls/crowbar.cpp ^
	../dlls/egon.cpp ^
	../dlls/gauss.cpp ^
	../dlls/handgrenade.cpp ^
	../dlls/hornetgun.cpp ^
	../dlls/mp5.cpp ^
	../dlls/python.cpp ^
	../dlls/rpg.cpp ^
	../dlls/satchel.cpp ^
	../dlls/shotgun.cpp ^
	../dlls/squeakgrenade.cpp ^
	../dlls/tripmine.cpp ^
	../dlls/glock.cpp ^
	../dlls/eagle.cpp ^
	../dlls/pipewrench.cpp ^
	../dlls/knife.cpp ^
	../dlls/grapple.cpp ^
	../dlls/m249.cpp ^
	../dlls/displacer.cpp ^
	../dlls/sniperrifle.cpp ^
	../dlls/shockrifle.cpp ^
	../dlls/sporelauncher.cpp ^
	../dlls/medkit.cpp ^
	ev_hldm.cpp ^
	hl/hl_baseentity.cpp ^
	hl/hl_events.cpp ^
	hl/hl_objects.cpp ^
	hl/hl_weapons.cpp ^
	achievements.cpp ^
	ammo.cpp ^
	ammo_secondary.cpp ^
	ammohistory.cpp ^
	battery.cpp ^
	cdll_int.cpp ^
	cl_msg_fx.cpp ^
	com_weapons.cpp ^
	death.cpp ^
	demo.cpp ^
	entity.cpp ^
	ev_common.cpp ^
	events.cpp ^
	flashlight.cpp ^
	GameStudioModelRenderer.cpp ^
	geiger.cpp ^
	health.cpp ^
	hud.cpp ^
	hud_caption.cpp ^
	hud_msg.cpp ^
	hud_redraw.cpp ^
	hud_spectator.cpp ^
	hud_update.cpp ^
	in_camera.cpp ^
	input.cpp ^
	input_goldsource.cpp ^
	input_mouse.cpp ^
	input_xash3d.cpp ^
	menu.cpp ^
	message.cpp ^
	nightvision.cpp ^
	parsemsg.cpp ^
	parsetext.cpp ^
	../pm_shared/pm_debug.c ^
	../pm_shared/pm_math.c ^
	../pm_shared/pm_shared.c ^
	../game_shared/tex_materials.c ^
	saytext.cpp ^
	status_icons.cpp ^
	statusbar.cpp ^
	studio_util.cpp ^
	StudioModelRenderer.cpp ^
	text_message.cpp ^
	train.cpp ^
	tri.cpp ^
	util.cpp ^
	view.cpp ^
	scoreboard.cpp ^
	MOTD.cpp
set DEFINES=/DCLIENT_DLL /DCLIENT_WEAPONS /Dsnprintf=_snprintf /DNO_VOICEGAMEMGR /DGOLDSOURCE_SUPPORT /DNDEBUG
set LIBS=user32.lib Winmm.lib
set OUTNAME=client.dll

cl %DEFINES% %LIBS% %SOURCES% %INCLUDES% -o %OUTNAME% /link /dll /out:%OUTNAME% /release

echo -- Compile done. Cleaning...

del *.obj *.exp *.lib *.ilk
echo -- Done.
