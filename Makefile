.PHONY: first
first: all
include dependencies/Makefile

# Directories.
OUTDIR_BIN=$(OUTDIR)
SRCDIR=./src
ASSETS=./assets
BINARIES=$(OUTDIR_BIN)/mobius

# Compilers and interpreters.
export PROTOC=$(PROTOBUF_DIR)/src/protoc
BLENDER_PATH=~/blender/blender

CFLAGS_EXTRA=\
  -std=c++11 \
  -isystem $(SFML_DIR)/include \
  -isystem $(PROTOBUF_DIR)/src \
  -isystem $(DEPENDENCIES)/glm
LFLAGS=\
  $(LFLAGSEXTRA) \
  $(PROTOBUF_DIR)/src/.libs/libprotobuf.a \
  $(SFML_DIR)/lib/libsfml-audio-s.a \
  $(SFML_DIR)/lib/libsfml-graphics-s.a \
  $(SFML_DIR)/lib/libsfml-window-s.a \
  $(SFML_DIR)/lib/libsfml-system-s.a \
  -lGLEW -lGL -lopenal -lsndfile -lfreetype -ljpeg \
  -lX11-xcb -lX11 -lxcb-image -lxcb-randr -lxcb -ludev -lpthread

# File listings.
CC_SOURCE_FILES=$(wildcard $(SRCDIR)/*.cc)
CC_TOOL_FILES=$(wildcard $(SRCDIR)/tools/*.cc)
PROTO_FILES=$(wildcard $(SRCDIR)/*.proto)
PROTO_TEXT_FILES=$(wildcard $(SRCDIR)/data/*.pb)
SHADER_FILES=$(wildcard $(SRCDIR)/shaders/*.glsl)
SHADER_H_FILES=$(wildcard $(SRCDIR)/shaders/*.glsl.h)
BLEND_FILES=$(wildcard $(ASSETS)/*.world.blend)
CC_TOOL_BINARIES=$(subst $(SRCDIR)/,$(GENDIR)/,$(CC_TOOL_FILES:.cc=))
CC_TOOL_OUTPUTS=$(subst $(SRCDIR)/,$(GENDIR)/,$(CC_TOOL_FILES:.cc=.h))
PROTO_OUTPUTS=$(subst $(SRCDIR)/,$(GENDIR)/,$(PROTO_FILES:.proto=.pb.cc))
SHADER_OUTPUTS=$(subst $(SRCDIR)/,$(GENDIR)/,$(SHADER_FILES:.glsl=.glsl.h))
PROTO_DATA_FILES=\
  $(subst $(SRCDIR)/,$(GENDIR)/,$(PROTO_TEXT_FILES)) \
  $(subst $(ASSETS)/,$(GENDIR)/data/,$(BLEND_FILES:.blend=.pb))
CC_GENERATED_FILES=$(PROTO_OUTPUTS)
H_GENERATED_FILES=$(SHADER_OUTPUTS) $(CC_TOOL_OUTPUTS)

H_FILES=$(wildcard $(SRCDIR)/*.h)
MISC_FILES=Makefile dependencies/Makefile
ALL_FILES=$(CC_SOURCE_FILES) $(H_FILES) $(MISC_FILES)

# Libraries. SHADER_OUTPUTS only necessary because we don't do include
# dependency generation for those (yet).
CC_OBJECT_FILE_PREREQS=\
  $(DEPENDENCIES)/sfml.build $(SHADER_OUTPUTS)

DISABLE_CC_DEPENDENCY_ANALYSIS=true
ifneq ('$(MAKECMDGOALS)', 'add')
ifneq ('$(MAKECMDGOALS)', 'todo')
ifneq ('$(MAKECMDGOALS)', 'wc')
ifneq ('$(MAKECMDGOALS)', 'clean')
ifneq ('$(MAKECMDGOALS)', 'clean_all')
ifneq ('$(MAKECMDGOALS)', 'data')
DISABLE_CC_DEPENDENCY_ANALYSIS=false
endif
endif
endif
endif
endif
endif
include dependencies/makelib/Makefile

# Master targets.
.PHONY: all
all: data $(BINARIES)
.PHONY: data
data: $(PROTO_DATA_FILES)
.PHONY: clean
clean:
	rm -rf $(OUTDIR) $(GENDIR)
.PHONY: clean_all
clean_all: clean clean_dependencies

# Binary linking.
$(BINARIES): $(OUTDIR_BIN)/%: \
  $(CC_OBJECT_FILES)
	$(MKDIR)
	@echo Linking ./$@
	$(CXX) -o ./$@ $(CC_OBJECT_FILES) $(LFLAGS)

# Tool compilation.
$(CC_TOOL_BINARIES): $(GENDIR)/%: \
  $(SRCDIR)/%.cc
	$(MKDIR)
	@echo Compiling ./$<
	$(CXX) $(CFLAGS) $(CFLAGS_EXTRA) $(WFLAGS) -o $@ ./$<

# Tool execution.
$(CC_TOOL_OUTPUTS): $(GENDIR)/%.h: \
  $(GENDIR)/%
	$(MKDIR)
	@echo Executing ./$<
	./$< > $@

# Proto files.
$(GENDIR)/%.pb.h: \
  $(GENDIR)/%.pb.cc
	touch $@ $<
$(GENDIR)/%.pb.cc: \
  $(SRCDIR)/%.proto $(DEPENDENCIES)/protobuf.build
	$(MKDIR)
	@echo Compiling ./$<
	$(PROTOC) --proto_path=$(SRCDIR) --cpp_out=$(GENDIR) ./$<

# Proto data files from data.
$(GENDIR)/%.pb: \
  $(SRCDIR)/%.pb $(PROTO_FILES)
	$(MKDIR)
	@echo Processing ./$<
	cat ./$< | $(PROTOC) --proto_path=$(SRCDIR) \
	    --encode=mobius.proto$(suffix $(basename $<)) \
	    $(PROTO_FILES) > $@ || (rm $@; false)

# Blender files.
$(GENDIR)/%.blend.pb: \
  $(ASSETS)/%.blend $(ASSETS)/export.py
	$(MKDIR)
	@echo Exporting ./$<
	EXPORT_PATH=$@ $(BLENDER_PATH) $< --background --python $(ASSETS)/export.py

# Proto files from blender.
$(GENDIR)/data/%.pb: \
  $(GENDIR)/%.blend.pb $(PROTO_FILES)
	$(MKDIR)
	@echo Processing ./$<
	cat ./$< | $(PROTOC) --proto_path=$(SRCDIR) \
	    --encode=mobius.proto$(suffix $(basename $(basename $<))) \
	    $(PROTO_FILES) > $@ || (rm $@; false)

# Shader files.
$(GENDIR)/%.glsl: \
  $(SRCDIR)/%.glsl $(SHADER_H_FILES)
	$(MKDIR)
	@echo Preprocessing ./$<
	echo "#version 330\n" > $@
	cpp $< >> $@

$(GENDIR)/%.glsl.h: \
  $(GENDIR)/%.glsl
	$(MKDIR)
	@echo Generating ./$@
	xxd -i $< > $@
