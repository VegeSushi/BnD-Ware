BUILD_DIR=build
include $(N64_INST)/include/n64.mk

CFLAGS += -Isrc/lua

EXCLUDE_LUA = src/lua/lua.c src/lua/onelua.c src/lua/luac.c src/lua/ltests.
SRCDIR = src src/lua
ALL_SRC = $(foreach d, $(SRCDIR), $(wildcard $(d)/*.c))
src = $(filter-out $(EXCLUDE_LUA), $(ALL_SRC))

assets_png = $(shell find assets -type f -name "*.png")
assets_conv = $(patsubst assets/%.png, filesystem/%.sprite, $(assets_png))

MKSPRITE_FLAGS ?=

all: bndware.z64

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) -f RGBA16 -o "$(dir $@)" "$<"

$(BUILD_DIR)/bndware.dfs: $(assets_conv)
$(BUILD_DIR)/bndware.elf: $(src:%.c=$(BUILD_DIR)/%.o)

bndware.z64: N64_ROM_TITLE="bndware"
bndware.z64: $(BUILD_DIR)/bndware.dfs

clean:
	rm -rf $(BUILD_DIR) filesystem/logo filesystem/menu filesystem/sprites bndware.z64

run: all
	gopher64 bndware.z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
