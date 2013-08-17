TARGET             := ufomodel

# if the linking should be static
$(TARGET)_STATIC   ?= $(STATIC)
ifeq ($($(TARGET)_STATIC),1)
$(TARGET)_LDFLAGS  += -static
endif

$(TARGET)_LINKER   := $(CC)
$(TARGET)_FILE     := $(TARGET)$(EXE_EXT)
$(TARGET)_LDFLAGS  += -lpng12 -ljpeg -lz -lm $(SDL_LIBS)
$(TARGET)_CFLAGS   += -DCOMPILE_MAP -ffloat-store $(SDL_CFLAGS) -I/mnt/utmp/codeblocks/usr/include/libpng12

ifeq ($(SSE),1)
   $(TARGET)_CFLAGS := $(filter-out -ffloat-store,$($(TARGET)_CFLAGS))
endif

$(TARGET)_SRCS      = \
	tools/ufomodel/ufomodel.c \
	\
	shared/mathlib.c \
	shared/mutex.c \
	shared/byte.c \
	shared/images.c \
	shared/parse.c \
	shared/shared.c \
	shared/utf8.c \
	\
	common/files.c \
	common/list.c \
	common/mem.c \
	common/unzip.c \
	common/ioapi.c \
	\
	client/renderer/r_model.c \
	client/renderer/r_model_alias.c \
	client/renderer/r_model_dpm.c \
	client/renderer/r_model_md2.c \
	client/renderer/r_model_md3.c \
	client/renderer/r_model_obj.c

ifeq ($(TARGET_OS),mingw32)
	$(TARGET)_SRCS+=\
		ports/windows/win_shared.c
else
	$(TARGET)_SRCS+= \
		ports/unix/unix_files.c \
		ports/unix/unix_shared.c \
		ports/unix/unix_main.c
endif

$(TARGET)_OBJS     := $(call ASSEMBLE_OBJECTS,$(TARGET))
$(TARGET)_CXXFLAGS := $($(TARGET)_CFLAGS)
$(TARGET)_CCFLAGS  := $($(TARGET)_CFLAGS)
