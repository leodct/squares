# Slopfile - thanks gemini 

TARGET_NAME = squares
BUILD_DIR   = bin
SRC_DIR     = src
OBJ_DIR     = obj
INC_DIR     = include
LIB_RLIMGUI = lib/rlimgui

# Compiler and Flags
CXX         = g++
CXXSTD      = -std=c++20
INCLUDES    = -I$(INC_DIR) -I$(LIB_RLIMGUI) -I/usr/include/imgui -I/usr/include
# Added -limgui here to link the system-wide ImGui package installed via yay
LIBS        = -lraylib -limgui -lGL -lm -lpthread -ldl -lrt -lX11

# Source Files
SRC_FILES   = $(wildcard $(SRC_DIR)/*.cpp)
RLIMGUI_SRC = $(LIB_RLIMGUI)/rlImGui.cpp

# Common compilation flags
CXXFLAGS_BASE = $(CXXSTD) $(INCLUDES) -MMD -MP

# --- Build Configurations ---
ifeq ($(BUILD),release)
    # --- RELEASE CONFIGURATION ---
    BUILD_SUBDIR   = release
    TARGET_SUFFIX  = _exe
    OPT_FLAGS      = -O3 -flto -DNDEBUG
    WARN_FLAGS     = -Wall -Wextra
    SANITIZE_FLAGS =
    STRIP_TARGET   = @strip $(EXECUTABLE)
else
    # --- DEBUG CONFIGURATION ---
    BUILD_SUBDIR   = debug
    TARGET_SUFFIX  = _debug_exe
    OPT_FLAGS      = -Og -g3 -DDEBUG
    WARN_FLAGS     = -Wall -Wextra -pedantic -Wshadow -Wformat=2 -Wconversion -Wnull-dereference -Wno-unused-parameter
    SANITIZE_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer
    STRIP_TARGET   =
endif

# Final Executable Path based on build mode
EXECUTABLE     = $(BUILD_DIR)/$(TARGET_NAME)$(TARGET_SUFFIX)

CXXFLAGS       = $(CXXFLAGS_BASE) $(OPT_FLAGS) $(WARN_FLAGS) $(SANITIZE_FLAGS)
CXXFLAGS_RL    = $(CXXFLAGS_BASE) $(OPT_FLAGS) -Wall -Wextra $(SANITIZE_FLAGS)
LDFLAGS        = $(OPT_FLAGS) $(SANITIZE_FLAGS) $(LIBS)

# Object Files Mapping (Only your project files + local rlImGui wrapper)
OBJ_PROJECT    = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/$(BUILD_SUBDIR)/src/%.cpp.o, $(SRC_FILES))
OBJ_RLIMGUI    = $(OBJ_DIR)/$(BUILD_SUBDIR)/lib/rlimgui/rlImGui.cpp.o

OBJECTS        = $(OBJ_PROJECT) $(OBJ_RLIMGUI)
DEPS           = $(OBJECTS:.o=.d)

# Default Target
all: debug

debug:
	@$(MAKE) BUILD=debug $(EXECUTABLE)

release:
	@$(MAKE) BUILD=release $(EXECUTABLE)

# Linking Target
$(EXECUTABLE): $(OBJECTS) | $(BUILD_DIR)
	@echo "==> Linking $(BUILD) build: $@"
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	$(STRIP_TARGET)
	@echo "==> Build complete successfully!"

# Compile Project Sources
$(OBJ_DIR)/$(BUILD_SUBDIR)/src/%.cpp.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)/$(BUILD_SUBDIR)/src
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile rlImGui Wrapper (using relaxed flags)
$(OBJ_DIR)/$(BUILD_SUBDIR)/lib/rlimgui/rlImGui.cpp.o: $(LIB_RLIMGUI)/rlImGui.cpp | $(OBJ_DIR)/$(BUILD_SUBDIR)/lib/rlimgui
	@echo "Compiling rlImGui..."
	$(CXX) $(CXXFLAGS_RL) -c $< -o $@

# Directory Generation Rules
$(BUILD_DIR):
	mkdir -p $@

$(OBJ_DIR)/$(BUILD_SUBDIR)/src:
	mkdir -p $@

$(OBJ_DIR)/$(BUILD_SUBDIR)/lib/rlimgui:
	mkdir -p $@

# Include Automatic Dependencies
-include $(DEPS)

# Utility Targets
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(OBJ_DIR) $(BUILD_DIR)

run: debug
	@./$(EXECUTABLE)

.PHONY: all debug release clean run