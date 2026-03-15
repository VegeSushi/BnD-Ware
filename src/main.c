#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <libdragon.h>
#include <stdbool.h>
#include <stdlib.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

surface_t* current_disp = NULL;
int g_total_score = 0;

typedef struct {
    char **filenames;
    int count;
} MinigameList;

MinigameList get_minigame_files(const char* dir_path) {
    MinigameList list = {NULL, 0};
    dir_t dir_info;
    
    if (dir_findfirst(dir_path, &dir_info) != 0) {
        return list;
    }

    int capacity = 16;
    list.filenames = malloc(capacity * sizeof(char*));

    do {
        if (strcmp(dir_info.d_name, ".") != 0 && strcmp(dir_info.d_name, "..") != 0) {
            
            if (list.count >= capacity) {
                capacity *= 2;
                list.filenames = realloc(list.filenames, capacity * sizeof(char*));
            }
            
            list.filenames[list.count] = strdup(dir_info.d_name);
            list.count++;
        }
        
    } while (dir_findnext(dir_path, &dir_info) == 0);

    for (int i = list.count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        char *temp = list.filenames[i];
        list.filenames[i] = list.filenames[j];
        list.filenames[j] = temp;
    }

    return list;
}

void free_minigame_list(MinigameList *list) {
    if (list->filenames != NULL) {
        for (int i = 0; i < list->count; i++) {
            free(list->filenames[i]);
        }
        free(list->filenames);
        list->filenames = NULL;
    }
    list->count = 0;
}

void run_minigame(lua_State* L, const char* minigame_path) {
    if (luaL_dofile(L, minigame_path) == 0) {
        
        if (lua_gettop(L) > 0 && lua_isnumber(L, -1)) {
            int game_score = lua_tointeger(L, -1);
            
            g_total_score += game_score;
            
            lua_pop(L, 1); 
            
            sprite_t *stars = sprite_load("rom:/sprites/stars.sprite");
            display_context_t disp = display_get();
            graphics_draw_sprite(disp, 0, 0, stars);
            
            char str[128];
            sprintf(str, "Game Score: %d\nTotal Score: %d", game_score, g_total_score);
            graphics_draw_text(disp, 20, 20, str);
            
            display_show(disp);
            sprite_free(stars);

            wait_ms(2500);
        }
    } else {
        display_context_t disp = display_get();
        graphics_fill_screen(disp, 0);
        
        graphics_draw_text(disp, 10, 10, "LUA ERROR:");
        graphics_draw_text(disp, 10, 30, lua_tostring(L, -1));
        display_show(disp);
        
        lua_pop(L, 1);
        
        while(1) {}
    }
}

int l_get_button(lua_State *L) {
    static const char *const button_names[] = {
        "A", "B", "Z", "START", 
        "UP", "DOWN", "LEFT", "RIGHT",
        "L", "R", 
        "C_UP", "C_DOWN", "C_LEFT", "C_RIGHT",
        NULL
    };

    int btn_index = luaL_checkoption(L, 1, NULL, button_names);

    joypad_poll();
    joypad_buttons_t btn = joypad_get_buttons(JOYPAD_PORT_1);
    
    bool is_pressed = false;

    switch (btn_index) {
        case 0:  is_pressed = btn.a; break;
        case 1:  is_pressed = btn.b; break;
        case 2:  is_pressed = btn.z; break;
        case 3:  is_pressed = btn.start; break;
        case 4:  is_pressed = btn.d_up; break;
        case 5:  is_pressed = btn.d_down; break;
        case 6:  is_pressed = btn.d_left; break;
        case 7:  is_pressed = btn.d_right; break;
        case 8:  is_pressed = btn.l; break;
        case 9:  is_pressed = btn.r; break;
        case 10: is_pressed = btn.c_up; break;
        case 11: is_pressed = btn.c_down; break;
        case 12: is_pressed = btn.c_left; break;
        case 13: is_pressed = btn.c_right; break;
    }

    lua_pushboolean(L, is_pressed); 
    return 1; 
}

int l_draw_sprite(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);

    if (current_disp != NULL) {
        sprite_t *sp = sprite_load(path);
        if (sp) {
            graphics_draw_sprite_trans(current_disp, x, y, sp);
            sprite_free(sp);
        }
    }
    return 0; 
}

int l_get_analog(lua_State *L) {
    joypad_poll();
    
    joypad_inputs_t inputs = joypad_get_inputs(JOYPAD_PORT_1);
    
    lua_pushinteger(L, inputs.stick_x); // Range is roughly -80 to +80
    lua_pushinteger(L, inputs.stick_y); 
    
    return 2; 
}

int l_draw_text(lua_State *L) {
    const char *text = luaL_checkstring(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);

    if (current_disp != NULL) {
        graphics_draw_text(current_disp, x, y, text);
    }
    return 0;
}

int l_clear_screen(lua_State *L) {
    int r = luaL_checkinteger(L, 1);
    int g = luaL_checkinteger(L, 2);
    int b = luaL_checkinteger(L, 3);
    int a = luaL_optinteger(L, 4, 255); 

    if (current_disp != NULL) {
        uint32_t color = graphics_make_color(r, g, b, a);
        graphics_fill_screen(current_disp, color);
    }
    return 0; 
}

int l_sleep(lua_State *L) {
    int ms = luaL_checkinteger(L, 1);
    
    wait_ms(ms);
    
    return 0; 
}

int l_begin_frame(lua_State *L) {
    current_disp = display_get();
    return 0;
}

int l_end_frame(lua_State *L) {
    if (current_disp) {
        display_show(current_disp);
        current_disp = NULL;
    }
    return 0;
}

int l_get_time_ms(lua_State *L) {
    uint32_t ms = timer_ticks() / (TICKS_PER_SECOND / 1000);
    
    lua_pushinteger(L, ms);
    return 1;
}

void draw_loading_screen(surface_t* disp, int progress) {
    graphics_fill_screen(disp, 0);
    graphics_draw_text(disp, 20, 20, "BNDWARE ENGINE INIT...");
    
    if (progress >= 1) graphics_draw_text(disp, 20, 40, "> Filesystem OK");
    if (progress >= 2) graphics_draw_text(disp, 20, 50, "> Joypad OK");
    if (progress >= 3) graphics_draw_text(disp, 20, 60, "> Lua VM OK");
    
    display_show(disp);
}

void show_about_screen() {
    surface_t* disp = display_get();

    sprite_t *stars = sprite_load("rom:/sprites/stars.sprite");
    graphics_draw_sprite(disp, 0, 0, stars);

    sprite_t *qr = sprite_load("rom:/sprites/qr.sprite");
    graphics_draw_sprite(disp, 100, 45, qr);

    graphics_draw_text(disp, 15, 20, "https://github.com/VegeSushi/BnD-Ware");

    display_show(disp);

    while (1) {
        joypad_poll();
        joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);

        if (keys.start || keys.a) {
            sprite_free(stars);
            sprite_free(qr);
            break;
        }
    }
}

int main(void) {
    console_init();

    timer_init();
    srand(get_ticks() & 0xFFFFFFFF);

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);

    surface_t* disp = display_get();

    graphics_fill_screen(disp, 0);

    dfs_init(DFS_DEFAULT_LOCATION);
    draw_loading_screen(disp, 1);

    joypad_init();
    disp = display_get();
    draw_loading_screen(disp, 2);

    lua_State *L = luaL_newstate();

    if (L) {
        luaL_openlibs(L);

        lua_register(L, "draw_sprite", l_draw_sprite);
        lua_register(L, "get_button", l_get_button);
        lua_register(L, "get_analog", l_get_analog);
        lua_register(L, "draw_text", l_draw_text);
        lua_register(L, "sleep", l_sleep);
        lua_register(L, "clear_screen", l_clear_screen);
        lua_register(L, "begin_frame", l_begin_frame);
        lua_register(L, "end_frame", l_end_frame);
        lua_register(L, "get_time_ms", l_get_time_ms);
    }

    disp = display_get();
    draw_loading_screen(disp, 3);

    wait_ms(500);

    char filename[27];

    for (int i = 0; i <= 59; i++) {
        sprintf(filename, "rom:/logo/logo_%04d.sprite", i);

        sprite_t *logo_sprite = sprite_load(filename);
        disp = display_get();
        graphics_draw_sprite(disp, 0, 0, logo_sprite);
        display_show(disp);
        wait_ms(1);
        sprite_free(logo_sprite);
    }

    while (1) {
        bool menu = true;

        int selected = 0;

        while (menu) {
            for (int i = 0; i <= 173; i++) {
                sprintf(filename, "rom:/menu/menu_%04d.sprite", i);

                sprite_t *menu_sprite = sprite_load(filename);
                disp = display_get();
                graphics_draw_sprite(disp, 0, 0, menu_sprite);
                graphics_draw_text(disp, 20, 20, "Bubbles And Doki Ware");
                display_show(disp);
                wait_ms(1);
                sprite_free(menu_sprite);

                joypad_poll();
        	    joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);

                if (selected == 0) {
                    graphics_draw_text(disp, 20, 60, "> Start");
                } else {
                    graphics_draw_text(disp, 20, 60, "  Start");
                } if (selected == 1) {
                    graphics_draw_text(disp, 20, 80, "> About");
                } else {
                    graphics_draw_text(disp, 20, 80, "  About");
                }

                if (keys.d_down) {
                    selected++;
                    if (selected > 1) selected = 0;
                }

                if (keys.d_up) {
                    selected--;
                    if (selected < 0) selected = 1;
                }

                if (keys.start || keys.a) {
                    if (selected == 0) {
                        g_total_score = 0;
                        menu = false;
                        break;
                    }
                    else if (selected == 1) {
                        show_about_screen();
                    }
                }
            }
        }

        MinigameList minigames = get_minigame_files("rom:/minigames");

        for (int i = 0; i < minigames.count; i++) {
            char full_path[256];
            snprintf(full_path, sizeof(full_path), "rom:/minigames/%s", minigames.filenames[i]);
            
            run_minigame(L, full_path);
        }
    
        free_minigame_list(&minigames);

        sprite_t *stars = sprite_load("rom:/sprites/stars.sprite");
        disp = display_get();
        graphics_draw_sprite(disp, 0, 0, stars);
            
        char str[128];
        sprintf(str, "Final Score: %d", g_total_score);
        graphics_draw_text(disp, 20, 20, str);
        display_show(disp);
        sprite_free(stars);
        wait_ms(5000);
    }
}
