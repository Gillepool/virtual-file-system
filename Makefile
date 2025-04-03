CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
INCLUDE_DIR = ./include
INCLUDE = -I$(INCLUDE_DIR)
PLUGINS_DIR = ./plugins

# macOS specific Qt configuration with Homebrew paths
QT_PREFIX = /opt/homebrew/opt/qt@5
QT_INCLUDE = $(QT_PREFIX)/include
QT_LIBS = $(QT_PREFIX)/lib
QT_BINS = $(QT_PREFIX)/bin

# Set environment variables for Qt
export PATH := $(QT_BINS):$(PATH)
export DYLD_LIBRARY_PATH := $(QT_LIBS):$(DYLD_LIBRARY_PATH)
export DYLD_FRAMEWORK_PATH := $(QT_LIBS):$(DYLD_FRAMEWORK_PATH)

# Qt tools
MOC = $(QT_BINS)/moc
UIC = $(QT_BINS)/uic

# Qt flags and libraries - adjusted for macOS
QT_CFLAGS = -I$(QT_INCLUDE) -I$(QT_INCLUDE)/QtWidgets -I$(QT_INCLUDE)/QtGui -I$(QT_INCLUDE)/QtCore
QT_LDFLAGS = -L$(QT_LIBS) -F$(QT_LIBS) -framework QtWidgets -framework QtGui -framework QtCore

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
LIB_DIR = lib
UI_DIR = ui
GENERATED_DIR = $(SRC_DIR)/generated

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
UI_FILES = $(wildcard $(UI_DIR)/*.ui)
UI_HEADERS = $(patsubst $(UI_DIR)/%.ui, $(GENERATED_DIR)/ui_%.h, $(UI_FILES))
PLUGIN_SOURCES = $(wildcard $(PLUGINS_DIR)/*.cpp)
PLUGIN_OBJECTS = $(patsubst $(PLUGINS_DIR)/%.cpp, $(OBJ_DIR)/plugins/%.o, $(PLUGIN_SOURCES))
PLUGIN_LIBRARIES = $(patsubst $(PLUGINS_DIR)/%.cpp, $(PLUGINS_DIR)/lib%.dylib, $(PLUGIN_SOURCES))

# Create a static library for the core VFS code
VFS_CORE_OBJECTS = $(OBJ_DIR)/FileNode.o $(OBJ_DIR)/VirtualFileSystem.o $(OBJ_DIR)/Compression.o $(OBJ_DIR)/Encryption.o
VFS_CORE_LIB = $(LIB_DIR)/libvfscore.a

# Shared library flags - platform specific
ifeq ($(shell uname), Darwin)
    # macOS
    SHARED_FLAGS = -dynamiclib -fPIC
    SHARED_EXT = .dylib
else ifeq ($(shell uname), Linux)
    # Linux
    SHARED_FLAGS = -shared -fPIC
    SHARED_EXT = .so
else
    # Windows
    SHARED_FLAGS = -shared
    SHARED_EXT = .dll
endif

# Get Qt headers with Q_OBJECT macro - Explicitly specify the headers with Q_OBJECT macro
MOC_HEADERS = $(INCLUDE_DIR)/QTerminal.h $(INCLUDE_DIR)/MainWindow.h
MOC_SOURCES = $(patsubst $(INCLUDE_DIR)/%.h, $(GENERATED_DIR)/moc_%.cpp, $(MOC_HEADERS))
MOC_OBJECTS = $(patsubst $(GENERATED_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(MOC_SOURCES))

# Define different object sets for CLI vs GUI
BASE_OBJECTS = $(OBJ_DIR)/Compression.o $(OBJ_DIR)/Encryption.o $(OBJ_DIR)/FileNode.o $(OBJ_DIR)/Shell.o \
               $(OBJ_DIR)/ShellAssistant.o $(OBJ_DIR)/VirtualFileSystem.o $(OBJ_DIR)/PluginManager.o

GUI_OBJECTS = $(BASE_OBJECTS) $(OBJ_DIR)/MainWindow.o $(OBJ_DIR)/QTerminal.o $(MOC_OBJECTS)
CLI_OBJECTS = $(BASE_OBJECTS) $(OBJ_DIR)/main_cli.o

TARGET = $(BIN_DIR)/vfs
TARGET_GUI = $(BIN_DIR)/vfs-gui

.PHONY: all clean gui cli mocs plugins

all: cli gui plugins

cli: directories $(TARGET)

gui: directories mocs $(UI_HEADERS) $(TARGET_GUI)

plugins: directories plugin_dirs $(VFS_CORE_LIB) $(PLUGIN_LIBRARIES)

directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) $(GENERATED_DIR) $(LIB_DIR)

plugin_dirs:
	@mkdir -p $(OBJ_DIR)/plugins $(PLUGINS_DIR)

mocs: directories $(MOC_SOURCES)

# Core VFS static library
$(VFS_CORE_LIB): $(VFS_CORE_OBJECTS)
	@mkdir -p $(LIB_DIR)
	ar rcs $@ $^

$(TARGET): $(BASE_OBJECTS) $(OBJ_DIR)/main_cli.o
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TARGET_GUI): $(GUI_OBJECTS) $(OBJ_DIR)/main.o
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) -o $@ $^ $(QT_LDFLAGS)

$(OBJ_DIR)/main_cli.o: $(SRC_DIR)/main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -D CLI_MODE_ONLY -c -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(QT_CFLAGS) -c -o $@ $<

$(OBJ_DIR)/plugins/%.o: $(PLUGINS_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)/plugins
	$(CXX) $(CXXFLAGS) $(INCLUDE) -fPIC -c -o $@ $<

$(OBJ_DIR)/moc_%.o: $(GENERATED_DIR)/moc_%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(QT_CFLAGS) -c -o $@ $<

$(GENERATED_DIR)/moc_%.cpp: $(INCLUDE_DIR)/%.h
	$(MOC) $(QT_CFLAGS) $< -o $@

$(GENERATED_DIR)/ui_%.h: $(UI_DIR)/%.ui
	$(UIC) $< -o $@

# Build plugin shared libraries - link against the VFS core library
$(PLUGINS_DIR)/lib%.dylib: $(OBJ_DIR)/plugins/%.o $(VFS_CORE_LIB)
	$(CXX) $(SHARED_FLAGS) -o $@ $< -L$(LIB_DIR) -lvfscore

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR) $(GENERATED_DIR) $(LIB_DIR) $(PLUGINS_DIR)/*.dylib $(PLUGINS_DIR)/*.so $(PLUGINS_DIR)/*.dll