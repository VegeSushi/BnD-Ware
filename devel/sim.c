#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h> // Added SDL_ttf
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define MICROGAMES_PATH "./filesystem/minigames/"

// Global renderer, window, font, and timing
SDL_Window* g_window = NULL;
SDL_Renderer* g_renderer = NULL;
TTF_Font* g_font = NULL;          // Global font
Uint32 g_frame_start_time = 0;    // For 60 FPS capping

// Sprite wrapper
typedef struct {
    SDL_Texture* tex;
    int w, h;
} sprite_t;

// Convert N64 path -> desktop path
void convert_path(const char* src, char* dst, size_t maxlen) {
    if (strncmp(src, "rom:/", 5) == 0) {
        snprintf(dst, maxlen, "assets/%s", src + 5);
    } else {
        strncpy(dst, src, maxlen);
        dst[maxlen-1] = 0;
    }
    size_t len = strlen(dst);
    if (len > 7 && strcmp(dst + len - 7, ".sprite") == 0) {
        strcpy(dst + len - 7, ".png");
    }
}

// Load PNG sprite
sprite_t* load_sprite(const char* path) {
    char final_path[512];
    convert_path(path, final_path, sizeof(final_path));

    SDL_Surface* surf = IMG_Load(final_path);
    if (!surf) {
        fprintf(stderr, "Failed to load %s: %s\n", final_path, IMG_GetError());
        return NULL;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(g_renderer, surf);
    sprite_t* sp = malloc(sizeof(sprite_t));
    sp->tex = tex;
    sp->w = surf->w;
    sp->h = surf->h;
    SDL_FreeSurface(surf);
    return sp;
}

void free_sprite(sprite_t* sp) {
    if (sp) {
        SDL_DestroyTexture(sp->tex);
        free(sp);
    }
}

// Lua bindings
int l_draw_sprite(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);

    sprite_t* sp = load_sprite(path);
    if (sp) {
        SDL_Rect dst = { x, y, sp->w, sp->h };
        SDL_RenderCopy(g_renderer, sp->tex, NULL, &dst);
        free_sprite(sp);
    }
    return 0;
}

// Updated text drawing using SDL_ttf
int l_draw_text(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);

    if (!g_font) return 0; // Don't crash if font failed to load

    SDL_Color color = {255, 255, 255, 255}; // White text
    SDL_Surface* text_surface = TTF_RenderText_Solid(g_font, text, color);
    
    if (text_surface) {
        SDL_Texture* text_texture = SDL_CreateTextureFromSurface(g_renderer, text_surface);
        SDL_Rect dest = { x, y, text_surface->w, text_surface->h };
        SDL_RenderCopy(g_renderer, text_texture, NULL, &dest);
        
        SDL_DestroyTexture(text_texture);
        SDL_FreeSurface(text_surface);
    }
    
    return 0;
}

int l_clear_screen(lua_State* L) {
    int r = luaL_checkinteger(L, 1);
    int g = luaL_checkinteger(L, 2);
    int b = luaL_checkinteger(L, 3);
    SDL_SetRenderDrawColor(g_renderer, r, g, b, 255);
    SDL_RenderClear(g_renderer);
    return 0;
}

int l_sleep(lua_State* L) {
    int ms = luaL_checkinteger(L, 1);
    SDL_Delay(ms);
    return 0;
}

// Capture frame start time
int l_begin_frame(lua_State* L) { 
    g_frame_start_time = SDL_GetTicks();
    return 0; 
}

// Present, process events, and cap framerate
int l_end_frame(lua_State* L) {
    SDL_RenderPresent(g_renderer);
    
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) exit(0);
    }

    // Framerate capping (Targeting ~60 FPS, which is ~16.6ms per frame)
    Uint32 frame_time = SDL_GetTicks() - g_frame_start_time;
    if (frame_time < 16) {
        SDL_Delay(16 - frame_time);
    }

    return 0;
}

int l_get_time_ms(lua_State* L) {
    lua_pushinteger(L, SDL_GetTicks());
    return 1;
}

int l_get_button(lua_State* L) {
    const char* btn = luaL_checkstring(L, 1);
    const Uint8* state = SDL_GetKeyboardState(NULL);
    bool pressed = false;

    if (strcmp(btn, "UP") == 0) pressed = state[SDL_SCANCODE_UP];
    else if (strcmp(btn, "DOWN") == 0) pressed = state[SDL_SCANCODE_DOWN];
    else if (strcmp(btn, "LEFT") == 0) pressed = state[SDL_SCANCODE_LEFT];
    else if (strcmp(btn, "RIGHT") == 0) pressed = state[SDL_SCANCODE_RIGHT];
    else if (strcmp(btn, "A") == 0) pressed = state[SDL_SCANCODE_Z];
    else if (strcmp(btn, "B") == 0) pressed = state[SDL_SCANCODE_X];
    else if (strcmp(btn, "START") == 0) pressed = state[SDL_SCANCODE_RETURN];

    lua_pushboolean(L, pressed);
    return 1;
}

// Initialize SDL and TTF
void init() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    IMG_Init(IMG_INIT_PNG);
    
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        exit(1);
    }

    g_window = SDL_CreateWindow("BNDWARE Microgame Simulator",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                320, 240, SDL_WINDOW_SHOWN);
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    SDL_RenderSetLogicalSize(g_renderer, 320, 240);

    // Load a font (You will need to provide this file!)
    g_font = TTF_OpenFont("assets/font.ttf", 16);
    if (!g_font) {
        fprintf(stderr, "Warning: Failed to load assets/font.ttf: %s\n", TTF_GetError());
    }
}

void shutdown() {
    if (g_font) TTF_CloseFont(g_font);
    TTF_Quit();
    
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    IMG_Quit();
    SDL_Quit();
}

// List Lua microgames in ../filesystem/minigames
int select_minigame(char* selected_file, size_t maxlen) {
    DIR* dir = opendir(MICROGAMES_PATH);
    if (!dir) {
        fprintf(stderr, "Failed to open microgames directory.\n");
        return 0;
    }

    struct dirent* entry;
    char* games[256];
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        size_t len = strlen(entry->d_name);
        if (len > 4 && strcmp(entry->d_name + len - 4, ".lua") == 0) {
            games[count] = strdup(entry->d_name);
            count++;
        }
    }
    closedir(dir);

    if (count == 0) {
        fprintf(stderr, "No microgames found.\n");
        return 0;
    }

    // Show menu
    printf("Select a microgame:\n");
    for (int i = 0; i < count; i++) {
        printf(" [%d] %s\n", i + 1, games[i]);
    }

    int choice = 0;
    while (choice < 1 || choice > count) {
        printf("Enter number: ");
        scanf("%d", &choice);
    }

    snprintf(selected_file, maxlen, "%s%s", MICROGAMES_PATH, games[choice - 1]);

    // Free memory
    for (int i = 0; i < count; i++) free(games[i]);

    return 1;
}

int main() {
    init();

    char minigame_path[512];
    if (!select_minigame(minigame_path, sizeof(minigame_path))) {
        shutdown();
        return 1;
    }

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    // Register functions
    lua_register(L, "draw_sprite", l_draw_sprite);
    lua_register(L, "draw_text", l_draw_text);
    lua_register(L, "clear_screen", l_clear_screen);
    lua_register(L, "sleep", l_sleep);
    lua_register(L, "begin_frame", l_begin_frame);
    lua_register(L, "end_frame", l_end_frame);
    lua_register(L, "get_time_ms", l_get_time_ms);
    lua_register(L, "get_button", l_get_button);

    // Run selected microgame
    if (luaL_dofile(L, minigame_path)) {
        fprintf(stderr, "Lua error: %s\n", lua_tostring(L, -1));
    } else {
        int final_score = 0;
        
        if (lua_gettop(L) > 0 && lua_isnumber(L, -1)) {
            final_score = lua_tointeger(L, -1);
            
            printf("\n====================================\n");
            printf("  Microgame Finished! \n");
            printf("  Final Score: %d\n", final_score);
            printf("====================================\n\n");
        } else {
            printf("\nMicrogame finished, but no score was returned.\n\n");
        }
    }

    lua_close(L);
    shutdown();
    return 0;
}