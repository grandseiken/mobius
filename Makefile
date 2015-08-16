.PHONY: first
first: all
include dependencies/Makefile

# Directories.
OUTDIR_BIN=$(OUTDIR)
SRCDIR=./src
BINARIES=$(OUTDIR_BIN)/mobius

# Compilers and interpreters.
export PROTOC=$(PROTOBUF_DIR)/src/protoc

CFLAGS_EXTRA=\
  -std=c++11 -isystem $(SFML_DIR)/include -isystem $(PROTOBUF_DIR)/src
LFLAGS=\
  $(LFLAGSEXTRA) \
  $(PROTOBUF_DIR)/src/.libs/libprotobuf.a \
  $(SFML_DIR)/lib/libsfml-audio-s.a \
  $(SFML_DIR)/lib/libsfml-graphics-s.a \
  $(SFML_DIR)/lib/libsfml-window-s.a \
  $(SFML_DIR)/lib/libsfml-system-s.a \
  -lGLEW -lGL -lopenal -lsndfile \
  -lX11-xcb -lX11 -lxcb-image -lxcb-randr -lxcb -ludev -lpthread

# File listings.
CC_SOURCE_FILES=$(wildcard $(SRCDIR)/*.cc)
PROTO_FILES=$(wildcard $(SRCDIR)/*.proto)
SHADER_FILES=$(wildcard $(SRCDIR)/shaders/*.glsl)
PROTO_OUTPUTS=$(subst $(SRCDIR)/,$(GENDIR)/,$(PROTO_FILES:.proto=.pb.cc))
SHADER_OUTPUTS=$(subst $(SRCDIR)/,$(GENDIR)/,$(SHADER_FILES:.glsl=.glsl.h))
CC_GENERATED_FILES=$(PROTO_OUTPUTS)

H_FILES=$(wildcard $(SRCDIR)/*.h)
MISC_FILES=Makefile dependencies/Makefile
ALL_FILES=$(CC_SOURCE_FILES) $(H_FILES) $(MISC_FILES)

# Libraries.
CC_OBJECT_FILE_PREREQS=\
  $(DEPENDENCIES)/sfml.build $(PROTO_OUTPUTS) $(SHADER_OUTPUTS)

DISABLE_CC_DEPENDENCY_ANALYSIS=true
ifneq ('$(MAKECMDGOALS)', 'add')
ifneq ('$(MAKECMDGOALS)', 'todo')
ifneq ('$(MAKECMDGOALS)', 'wc')
ifneq ('$(MAKECMDGOALS)', 'clean')
ifneq ('$(MAKECMDGOALS)', 'clean_all')
DISABLE_CC_DEPENDENCY_ANALYSIS=false
endif
endif
endif
endif
endif
include dependencies/makelib/Makefile

# Master targets.
.PHONY: all
all: $(BINARIES)
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

# Proto files.
$(GENDIR)/%.pb.h: \
  $(GENDIR)/%.pb.cc
	touch $@ $<
$(GENDIR)/%.pb.cc: \
  $(SRCDIR)/%.proto $(DEPENDENCIES)/protobuf.build
	$(MKDIR)
	@echo Compiling ./$<
	$(PROTOC) --proto_path=$(SRCDIR) --cpp_out=$(GENDIR) ./$<

# Shader files.
$(GENDIR)/%.glsl.h: \
  $(SRCDIR)/%.glsl
	$(MKDIR)
	@echo Compiling ./$<
	xxd -i $< > $@
