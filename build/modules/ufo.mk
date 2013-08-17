TARGET             := ufo

# if the linking should be static
$(TARGET)_STATIC   ?= $(STATIC)
ifeq ($($(TARGET)_STATIC),1)
$(TARGET)_LDFLAGS  += -static
endif

$(TARGET)_LINKER   := $(CC)
$(TARGET)_FILE     := $(TARGET)$(EXE_EXT)
$(TARGET)_CFLAGS   += -DCOMPILE_UFO $(BFD_CFLAGS) $(SDL_CFLAGS) $(SDL_TTF_CFLAGS) $(SDL_MIXER_CFLAGS) $(CURL_CFLAGS) $(THEORA_CFLAGS) $(XVID_CFLAGS) $(VORBIS_CFLAGS) $(OGG_CFLAGS) $(MXML_CFLAGS) -I/mnt/utmp/codeblocks/usr/include/libpng14
$(TARGET)_LDFLAGS  += -lpng14 -ljpeg $(BFD_LIBS) $(INTL_LIBS) $(SDL_TTF_LIBS) $(SDL_MIXER_LIBS) $(OPENGL_LIBS) $(SDL_LIBS) $(CURL_LIBS) $(THEORA_LIBS) $(XVID_LIBS) $(VORBIS_LIBS) $(OGG_LIBS) $(MXML_LIBS) $(SO_LIBS) -lz

$(TARGET)_SRCS      = \
	client/cl_console.c \
	client/cl_http.c \
	client/cl_inventory.c \
	client/cl_inventory_callbacks.c \
	client/cl_irc.c \
	client/cl_language.c \
	client/cl_main.c \
	client/cl_menu.c \
	client/cl_screen.c \
	client/cl_team.c \
	client/cl_tip.c \
	client/cl_tutorials.c \
	client/cl_video.c \
	\
	client/input/cl_input.c \
	client/input/cl_joystick.c \
	client/input/cl_keys.c \
	\
	client/cinematic/cl_cinematic.c \
	client/cinematic/cl_cinematic_roq.c \
	client/cinematic/cl_cinematic_ogm.c \
	client/cinematic/cl_sequence.c \
	\
	client/battlescape/cl_actor.c \
	client/battlescape/cl_battlescape.c \
	client/battlescape/cl_camera.c \
	client/battlescape/cl_hud.c \
	client/battlescape/cl_hud_callbacks.c \
	client/battlescape/cl_localentity.c \
	client/battlescape/cl_parse.c \
	client/battlescape/cl_particle.c \
	client/battlescape/cl_radar.c \
	client/battlescape/cl_ugv.c \
	client/battlescape/cl_view.c \
	client/battlescape/cl_spawn.c \
	\
	client/battlescape/events/e_main.c \
	client/battlescape/events/e_parse.c \
	client/battlescape/events/e_server.c \
	client/battlescape/events/event/actor/e_event_actoradd.c \
	client/battlescape/events/event/actor/e_event_actorappear.c \
	client/battlescape/events/event/actor/e_event_actorclientaction.c \
	client/battlescape/events/event/actor/e_event_actordie.c \
	client/battlescape/events/event/actor/e_event_actormove.c \
	client/battlescape/events/event/actor/e_event_actorresetclientaction.c \
	client/battlescape/events/event/actor/e_event_actorreservationchange.c \
	client/battlescape/events/event/actor/e_event_actorreactionfirechange.c \
	client/battlescape/events/event/actor/e_event_actorrevitalised.c \
	client/battlescape/events/event/actor/e_event_actorshoot.c \
	client/battlescape/events/event/actor/e_event_actorshoothidden.c \
	client/battlescape/events/event/actor/e_event_actorstartshoot.c \
	client/battlescape/events/event/actor/e_event_actorstatechange.c \
	client/battlescape/events/event/actor/e_event_actorstats.c \
	client/battlescape/events/event/actor/e_event_actorthrow.c \
	client/battlescape/events/event/actor/e_event_actorturn.c \
	client/battlescape/events/event/inventory/e_event_invadd.c \
	client/battlescape/events/event/inventory/e_event_invammo.c \
	client/battlescape/events/event/inventory/e_event_invdel.c \
	client/battlescape/events/event/inventory/e_event_invreload.c \
	client/battlescape/events/event/player/e_event_centerview.c \
	client/battlescape/events/event/player/e_event_doendround.c \
	client/battlescape/events/event/player/e_event_endroundannounce.c \
	client/battlescape/events/event/player/e_event_reset.c \
	client/battlescape/events/event/player/e_event_results.c \
	client/battlescape/events/event/player/e_event_startgame.c \
	client/battlescape/events/event/world/e_event_addbrushmodel.c \
	client/battlescape/events/event/world/e_event_addedict.c \
	client/battlescape/events/event/world/e_event_doorclose.c \
	client/battlescape/events/event/world/e_event_dooropen.c \
	client/battlescape/events/event/world/e_event_entappear.c \
	client/battlescape/events/event/world/e_event_entdestroy.c \
	client/battlescape/events/event/world/e_event_entperish.c \
	client/battlescape/events/event/world/e_event_explode.c \
	client/battlescape/events/event/world/e_event_particleappear.c \
	client/battlescape/events/event/world/e_event_particlespawn.c \
	client/battlescape/events/event/world/e_event_sound.c \
	\
	client/sound/s_music.c \
	client/sound/s_main.c \
	client/sound/s_mix.c \
	client/sound/s_sample.c \
	\
	client/cgame/cl_game.c \
	client/cgame/cl_game_team.c \
	\
	client/ui/ui_actions.c \
	client/ui/ui_behaviour.c \
	client/ui/ui_components.c \
	client/ui/ui_data.c \
	client/ui/ui_dragndrop.c \
	client/ui/ui_draw.c \
	client/ui/ui_expression.c \
	client/ui/ui_font.c \
	client/ui/ui_sprite.c \
	client/ui/ui_input.c \
	client/ui/ui_main.c \
	client/ui/ui_nodes.c \
	client/ui/ui_parse.c \
	client/ui/ui_popup.c \
	client/ui/ui_render.c \
	client/ui/ui_sound.c \
	client/ui/ui_timer.c \
	client/ui/ui_tooltip.c \
	client/ui/ui_windows.c \
	client/ui/node/ui_node_abstractnode.c \
	client/ui/node/ui_node_abstractvalue.c \
	client/ui/node/ui_node_abstractoption.c \
	client/ui/node/ui_node_abstractscrollbar.c \
	client/ui/node/ui_node_abstractscrollable.c \
	client/ui/node/ui_node_bar.c \
	client/ui/node/ui_node_base.c \
	client/ui/node/ui_node_baseinventory.c \
	client/ui/node/ui_node_battlescape.c \
	client/ui/node/ui_node_button.c \
	client/ui/node/ui_node_checkbox.c \
	client/ui/node/ui_node_data.c \
	client/ui/node/ui_node_video.c \
	client/ui/node/ui_node_container.c \
	client/ui/node/ui_node_controls.c \
	client/ui/node/ui_node_custombutton.c \
	client/ui/node/ui_node_editor.c \
	client/ui/node/ui_node_ekg.c \
	client/ui/node/ui_node_image.c \
	client/ui/node/ui_node_item.c \
	client/ui/node/ui_node_keybinding.c \
	client/ui/node/ui_node_linechart.c \
	client/ui/node/ui_node_map.c \
	client/ui/node/ui_node_material_editor.c \
	client/ui/node/ui_node_model.c \
	client/ui/node/ui_node_messagelist.c \
	client/ui/node/ui_node_option.c \
	client/ui/node/ui_node_optionlist.c \
	client/ui/node/ui_node_optiontree.c \
	client/ui/node/ui_node_panel.c \
	client/ui/node/ui_node_radar.c \
	client/ui/node/ui_node_radiobutton.c \
	client/ui/node/ui_node_rows.c \
	client/ui/node/ui_node_selectbox.c \
	client/ui/node/ui_node_sequence.c \
	client/ui/node/ui_node_string.c \
	client/ui/node/ui_node_special.c \
	client/ui/node/ui_node_spinner.c \
	client/ui/node/ui_node_spinner2.c \
	client/ui/node/ui_node_tab.c \
	client/ui/node/ui_node_tbar.c \
	client/ui/node/ui_node_text.c \
	client/ui/node/ui_node_text2.c \
	client/ui/node/ui_node_textlist.c \
	client/ui/node/ui_node_textentry.c \
	client/ui/node/ui_node_texture.c \
	client/ui/node/ui_node_todo.c \
	client/ui/node/ui_node_vscrollbar.c \
	client/ui/node/ui_node_window.c \
	client/ui/node/ui_node_zone.c \
	\
	common/binaryexpressionparser.c \
	common/cmd.c \
	common/http.c \
	common/ioapi.c \
	common/unzip.c \
	common/bsp.c \
	common/grid.c \
	common/cmodel.c \
	common/common.c \
	common/cvar.c \
	common/files.c \
	common/list.c \
	common/md4.c \
	common/md5.c \
	common/mem.c \
	common/msg.c \
	common/net.c \
	common/netpack.c \
	common/dbuffer.c \
	common/pqueue.c \
	common/scripts.c \
	common/tracing.c \
	common/routing.c \
	common/xml.c \
	\
	server/sv_ccmds.c \
	server/sv_game.c \
	server/sv_init.c \
	server/sv_log.c \
	server/sv_main.c \
	server/sv_mapcycle.c \
	server/sv_rma.c \
	server/sv_send.c \
	server/sv_user.c \
	server/sv_world.c \
	\
	client/renderer/r_array.c \
	client/renderer/r_bsp.c \
	client/renderer/r_draw.c \
	client/renderer/r_corona.c \
	client/renderer/r_entity.c \
	client/renderer/r_font.c \
	client/renderer/r_flare.c \
	client/renderer/r_framebuffer.c \
	client/renderer/r_geoscape.c \
	client/renderer/r_image.c \
	client/renderer/r_light.c \
	client/renderer/r_lightmap.c \
	client/renderer/r_main.c \
	client/renderer/r_material.c \
	client/renderer/r_matrix.c \
	client/renderer/r_misc.c \
	client/renderer/r_mesh.c \
	client/renderer/r_mesh_anim.c \
	client/renderer/r_model.c \
	client/renderer/r_model_alias.c \
	client/renderer/r_model_brush.c \
	client/renderer/r_model_dpm.c \
	client/renderer/r_model_md2.c \
	client/renderer/r_model_md3.c \
	client/renderer/r_model_obj.c \
	client/renderer/r_particle.c \
	client/renderer/r_program.c \
	client/renderer/r_sdl.c \
	client/renderer/r_surface.c \
	client/renderer/r_state.c \
	client/renderer/r_sphere.c \
	client/renderer/r_thread.c \
	\
	shared/bfd.c \
	shared/byte.c \
	shared/mathlib.c \
	shared/mathlib_extra.c \
	shared/mutex.c \
	shared/utf8.c \
	shared/images.c \
	shared/stringhunk.c \
	shared/infostring.c \
	shared/parse.c \
	shared/shared.c \
	\
	game/q_shared.c \
	game/chr_shared.c \
	game/inv_shared.c \
	game/inventory.c \
	\
	$(MXML_SRCS)

ifneq ($(findstring $(TARGET_OS), netbsd freebsd linux),)
	$(TARGET)_SRCS += \
		ports/linux/linux_main.c \
		ports/unix/unix_console.c \
		ports/unix/unix_files.c \
		ports/unix/unix_shared.c \
		ports/unix/unix_main.c
	$(TARGET)_LDFLAGS +=
endif

ifeq ($(TARGET_OS),mingw32)
	$(TARGET)_SRCS +=\
		ports/windows/win_backtrace.c \
		ports/windows/win_console.c \
		ports/windows/win_shared.c \
		ports/windows/win_main.c \
		ports/windows/ufo.rc
	$(TARGET)_LDFLAGS +=
endif

ifeq ($(TARGET_OS),darwin)
	$(TARGET)_SRCS += \
		ports/macosx/osx_main.m \
		ports/unix/unix_console.c \
		ports/unix/unix_files.c \
		ports/unix/unix_shared.c \
		ports/unix/unix_main.c
	$(TARGET)_LDFLAGS +=
endif

ifeq ($(TARGET_OS),solaris)
	$(TARGET)_SRCS += \
		ports/solaris/solaris_main.c \
		ports/unix/unix_console.c \
		ports/unix/unix_files.c \
		ports/unix/unix_shared.c \
		ports/unix/unix_main.c
	$(TARGET)_LDFLAGS +=
endif

ifeq ($(TARGET_OS),android)
	$(TARGET)_SRCS += \
		ports/android/android_console.c \
		ports/android/android_debugger.c \
		ports/android/android_main.c \
		ports/android/android_system.c \
		ports/unix/unix_shared.c \
		ports/unix/unix_files.c
endif

ifeq ($(HARD_LINKED_GAME),1)
	$(TARGET)_SRCS     += $(game_SRCS)
else
	$(TARGET)_DEPS     := game
endif

ifeq ($(HARD_LINKED_CGAME),1)
	$(TARGET)_SRCS     += \
		$(cgame-campaign_SRCS) \
		$(cgame-skirmish_SRCS) \
		$(cgame-multiplayer_SRCS) \
		$(cgame-staticcampaign_SRCS)
else
	$(TARGET)_DEPS     := \
		cgame-campaign \
		cgame-skirmish \
		cgame-multiplayer \
		cgame-staticcampaign
endif

#Pandora begin
$(TARGET)_SRCS     += \
		client/renderer/eglport.c
#Pandora end

$(TARGET)_OBJS     := $(call ASSEMBLE_OBJECTS,$(TARGET))
$(TARGET)_CXXFLAGS := $($(TARGET)_CFLAGS)
$(TARGET)_CCFLAGS  := $($(TARGET)_CFLAGS)
