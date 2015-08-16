# Dependency directories.
DEPENDENCIES=./dependencies
PROTOBUF_DIR=$(DEPENDENCIES)/protobuf
SFML_DIR=$(DEPENDENCIES)/sfml

# CMake.
CMAKE=cmake
SFML_CMAKE_FLAGS= \
  -DCMAKE_CC_COMPILER=$(CC) \
  -DCMAKE_CXX_COMPILER=$(CXX) \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=. \
  -DBUILD_SHARED_LIBS=FALSE \
  -DSFML_BUILD_DOC=FALSE \
  -DSFML_BUILD_EXAMPLES=FALSE \
  -DSFML_INSTALL_PKGCONFIG_FILES=FALSE

# Build Protobuf.
$(DEPENDENCIES)/protobuf.build:
	@echo Building Protobuf
	cd $(PROTOBUF_DIR) && ./autogen.sh
	cd $(PROTOBUF_DIR) && DIST_LANG=cpp ./configure
	cd $(PROTOBUF_DIR) && DIST_LANG=cpp $(MAKE)
	touch $@

# Build SFML.
$(DEPENDENCIES)/sfml.build:
	@echo Building SFML
	cd $(SFML_DIR) && $(CMAKE) $(SFML_CMAKE_FLAGS)
	cd $(SFML_DIR) && $(MAKE)
	touch $@

.PHONY: clean_dependencies
clean_dependencies:
	rm -f $(DEPENDENCIES)/*.build $(DEPENDENCIES)/.build
	rm -rf $(PROTOBUF_DIR) $(SFML_DIR)
	git submodule update
