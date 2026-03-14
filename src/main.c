#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <libdragon.h>
#include <stdbool.h>
#include <stdio.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

surface_t* current_disp = NULL;
int g_total_score = 0;

void run_minigame(lua_State* L, const char* minigame_path) {
    if (luaL_dofile(L, minigame_path) == 0) {
        
        if (lua_gettop(L) > 0 && lua_isnumber(L, -1)) {
            int game_score = lua_tointeger(L, -1);
            
            g_total_score += game_score;
            
            lua_pop(L, 1); 
            
            display_context_t disp = display_get();
            graphics_fill_screen(disp, 0); 
            
            char str[128];
            sprintf(str, "Game Score: %d\nTotal Score: %d", game_score, g_total_score);
            graphics_draw_text(disp, 20, 20, str);
            
            display_show(disp); 
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

int main(void) {
    console_init();

    timer_init();

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);

    surface_t* disp = display_get();

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

    bool menu = true;

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

            if (keys.start) {
                menu = false;
                break;
            }
        }
    }

    run_minigame(L, "rom:/minigames/pop.lua");
}
