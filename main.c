#include <SDL2/SDL.h>
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {
    A_NONE,
    A_MOVE_RIGHT,
    A_MOVE_LEFT,
    A_MOVE_DOWN,
    A_MOVE_UP,
} Actions;

typedef enum {
    GS_PLAYING,
    GS_LOST,
    GS_WON,
} GameStates;

typedef struct {
    int board[4 * 4];
    GameStates state;
    Actions action;
    int score, moves;
    int last_inserted;
} Game;

bool has_won(Game* game)
{
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            if (game->board[y * 4 + x] == 11)
                return true;
    return false;
}

bool has_lost(Game* game)
{
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            if (game->board[y * 4 + x] == 0)
                return false;
    for (int a = 0; a < 3; a++)
        for (int b = 0; b < 4; b++)
            if (game->board[a * 4 + b] == game->board[(a + 1) * 4 + b]
                || game->board[b * 4 + a] == game->board[b * 4 + a + 1])
                return false;
    return true;
}

int empty_tiles(Game* game)
{
    int amount = 0;
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            if (game->board[y * 4 + x] == 0)
                amount++;
    return amount;
}

int add_random_tile(Game* game)
{
    int tiles = empty_tiles(game);
    int tile = ((rand() / 16) * tiles) / (RAND_MAX / 16);
    int n = 0;
    for (int i = 0; i < 16; i++) {
        if (game->board[i] == 0) {
            if (n == tile) {
                game->board[i] = rand() > RAND_MAX / 3 * 2 ? 2 : 1;
                return i;
            } else {
                n++;
            }
        }
    }
    return -1;
}

bool move_cell_maybe_break(int* target, int* src)
{
    if (*target == 0) {
        *target = *src;
        *src = 0;
    } else if (*target == *src) {
        *target += 1;
        *src = 0;
        return true;
    } else if (*src != 0) {
        return true;
    }
    return false;
}

void move_right(Game* game)
{
    for (int y = 0; y < 4; y++)
        for (int ix = 3; ix >= 0; ix--)
            for (int jx = ix - 1; jx >= 0; jx--)
                if (move_cell_maybe_break(&game->board[y * 4 + ix], &game->board[y * 4 + jx]))
                    break;
}

void move_left(Game* game)
{
    for (int y = 0; y < 4; y++)
        for (int ix = 0; ix < 3; ix++)
            for (int jx = ix + 1; jx < 4; jx++)
                if (move_cell_maybe_break(&game->board[y * 4 + ix], &game->board[y * 4 + jx]))
                    break;
}

void move_down(Game* game)
{
    for (int x = 0; x < 4; x++)
        for (int iy = 3; iy >= 0; iy--)
            for (int jy = iy - 1; jy >= 0; jy--)
                if (move_cell_maybe_break(&game->board[iy * 4 + x], &game->board[jy * 4 + x]))
                    break;
}

void move_up(Game* game)
{
    for (int x = 0; x < 4; x++)
        for (int iy = 0; iy < 3; iy++)
            for (int jy = iy + 1; jy < 4; jy++)
                if (move_cell_maybe_break(&game->board[iy * 4 + x], &game->board[jy * 4 + x]))
                    break;
}

void handle_action(Game* game)
{
    switch (game->action) {
    case A_MOVE_RIGHT:
        move_right(game);
        break;
    case A_MOVE_LEFT:
        move_left(game);
        break;
    case A_MOVE_DOWN:
        move_down(game);
        break;
    case A_MOVE_UP:
        move_up(game);
        break;
    default:
        break;
    }
}

int calculate_score(Game* game)
{
    int score = 0;
    for (int i = 0; i < 16; i++)
        score += pow(2, game->board[i]);
    return score;
}

void update(Game* game, double delta)
{
    if (game->state == GS_PLAYING) {
        handle_action(game);
        if (game->action != A_NONE) {
            game->action = A_NONE;
            game->last_inserted = add_random_tile(game);
            game->moves++;
        }
        if (has_won(game))
            game->state = GS_WON;
        else if (has_lost(game))
            game->state = GS_LOST;
    }
    game->score = calculate_score(game);
}

SDL_Color tile_color(Game* game, int x, int y)
{
#ifdef COLOR_INSERTED_TILE
    if (y * 4 + x == game->last_inserted)
        return (SDL_Color) { 0x00, 0xFF, 0x00, 0 };
#endif
    int tile_value = game->board[y * 4 + x];
    return tile_value > 0
        ? (SDL_Color) { 0xFF, 0x44 + 0x11 * tile_value, 0xBB - 0x11 * tile_value, 0 }
        : (SDL_Color) { 0x88, 0x88, 0x88, 0 };
}

typedef struct {
    int x, w;
} TextCenterOffset;

TextCenterOffset text_center_offset(int value)
{
    switch (value) {
    case 1:
    case 2:
    case 3:
        return (TextCenterOffset) { .x = 30, .w = 60 };
    case 4:
    case 5:
    case 6:
        return (TextCenterOffset) { .x = 15, .w = 30 };
    case 7:
    case 8:
    case 9:
        return (TextCenterOffset) { .x = 7, .w = 14 };
    case 10:
    case 11:
    default:
        return (TextCenterOffset) { .x = 0, .w = 0 };
    }
}

void render_tile(Game* game, SDL_Renderer* renderer, TTF_Font* font, int x, int y)
{
    SDL_Color color = tile_color(game, x, y);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 0);
    SDL_RenderFillRect(renderer,
        &(SDL_Rect) {
            .x = x * 128 + 2,
            .y = y * 128 + 2,
            .w = 128 - 4,
            .h = 128 - 4,
        });
    if (game->board[y * 4 + x] != 0) {
        char buffer[5] = { 0 };
        snprintf(buffer, 5, "%.0lf", pow(2, game->board[y * 4 + x]));
        SDL_Surface* textSurface
            = TTF_RenderUTF8_Shaded(font, buffer, (SDL_Color) { 0, 0, 0, 0 }, color);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        TextCenterOffset offset = text_center_offset(game->board[y * 4 + x]);
        SDL_RenderCopy(renderer, textTexture, NULL,
            &(SDL_Rect) {
                x * 128 + 16 + offset.x,
                y * 128 + 40,
                128 - 32 - offset.w,
                128 - 80,
            });
        SDL_DestroyTexture(textTexture);
        SDL_FreeSurface(textSurface);
    }
}

void render_splash_screen(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &(SDL_Rect) { 0, 0, 512, 512 });
    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(font, text, (SDL_Color) { 0, 0, 0, 0 });
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_RenderCopy(renderer, textTexture, NULL, &(SDL_Rect) { 50, 120, 400, 80 });
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

void render_score_and_moves(Game* game, SDL_Renderer* renderer, TTF_Font* font)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    char text[32] = { 0 };
    snprintf(text, 32, "Score: %d", game->score);
    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(font, text, (SDL_Color) { 0, 0, 0, 0 });
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_RenderCopy(renderer, textTexture, NULL, &(SDL_Rect) { 50, 200, 400, 80 });
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);

    snprintf(text, 32, "Moves: %d", game->moves);
    textSurface = TTF_RenderUTF8_Blended(font, text, (SDL_Color) { 0, 0, 0, 0 });
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_RenderCopy(renderer, textTexture, NULL, &(SDL_Rect) { 50, 280, 400, 80 });
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

void render(Game* game, SDL_Renderer* renderer, TTF_Font* font)
{
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            render_tile(game, renderer, font, x, y);
        }
    }
    if (game->state == GS_WON) {
        render_splash_screen(
            renderer, font, "YOU HAVE WON", (SDL_Color) { 0x88, 0xFF, 0x00, 0x88 });
        render_score_and_moves(game, renderer, font);
    } else if (game->state == GS_LOST) {
        render_splash_screen(
            renderer, font, "YOU HAVE LOST", (SDL_Color) { 0x88, 0x88, 0x88, 0x88 });
        render_score_and_moves(game, renderer, font);
    }
}

bool pull_events_keep_running(SDL_Event* event, Game* game)
{
    while (SDL_PollEvent(event)) {
        if (event->type == SDL_QUIT) {
            return false;
        } else if (event->type == SDL_KEYDOWN) {
            switch (event->key.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE:
                return false;
            case SDL_SCANCODE_RIGHT:
                game->action = A_MOVE_RIGHT;
                break;
            case SDL_SCANCODE_LEFT:
                game->action = A_MOVE_LEFT;
                break;
            case SDL_SCANCODE_DOWN:
                game->action = A_MOVE_DOWN;
                break;
            case SDL_SCANCODE_UP:
                game->action = A_MOVE_UP;
                break;
            default:
                break;
            }
        }
    }
    return true;
}

int main(int argc, char** argv)
{
    srand(time(NULL));
    Game game = {
        .board = { 0 },
        .state = GS_PLAYING,
        .action = A_NONE,
        .moves = 0,
        .score = 0,
        .last_inserted = -1,
    };
    add_random_tile(&game);
    add_random_tile(&game);

    SDL_Event event;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window;
    SDL_Renderer* renderer;
    if (SDL_CreateWindowAndRenderer(512, 512, 0, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s",
            SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL_ttf: %s", SDL_GetError());
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("font.ttf", 36);
    if (!font) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load font: %s", SDL_GetError());
        return 1;
    }

    Uint64 time_before = SDL_GetTicks64();
    bool should_run = true;
    while (should_run) {
        Uint64 time_now = SDL_GetTicks64();
        double delta = (time_now - time_before) / (double)1000;
        time_before = time_now;

        if (!pull_events_keep_running(&event, &game))
            break;

        update(&game, delta);

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        render(&game, renderer, font);
        SDL_RenderPresent(renderer);

        char titleText[32] = { 0 };
        snprintf(titleText, 32, "2048 - Score = %d, Moves = %d", game.score, game.moves);
        SDL_SetWindowTitle(window, titleText);

        SDL_Delay(6);
    }

    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
