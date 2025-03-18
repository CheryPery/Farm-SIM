#include "raylib.h"
#include "raymath.h"

#define TILE_HEIGHT 8
#define TILE_WIDTH 8

#define MAX_TEXTURES 1
typedef enum{
    TEXTURE_TILEMAP = 0
} Texture_asset;

Texture2D textures[MAX_TEXTURES];


#define WORLD_WIDTH 20
#define WORLD_HEIGHT 20

typedef struct {
    int x;
    int y;
} sTile;

sTile world[WORLD_WIDTH][WORLD_HEIGHT];



void Game_Startup() {

    InitAudioDevice();

    Image image = LoadImage("assets/colored_tilemap_packed.png");
    textures[TEXTURE_TILEMAP] = LoadTextureFromImage(image);
    UnloadImage(image);

    for(int i = 0; i < WORLD_WIDTH; i++) {
        for(int j = 0; j < WORLD_HEIGHT; j++) {
            world[i][j] = (sTile)
            {
                .x = i,
                .y = j,
            };
        }
    }
}

void Game_Update() {

}

void Game_Render() {

    sTile tile;
    int texture_index_x = 0;
    int texture_index_y = 0;

    for(int i = 0; i < WORLD_WIDTH; i++) {
        for(int j = 0; j < WORLD_HEIGHT; j++) {
            tile = world[i][j];
            texture_index_x = 4; 
            texture_index_y = 4;

            Rectangle source = { (float)texture_index_x, (float)texture_index_y, (float)TILE_WIDTH, (float)TILE_HEIGHT };
            Rectangle dest = { (float)(tile.x * TILE_WIDTH), (float)(tile.y * TILE_HEIGHT), (float)TILE_WIDTH, (float)TILE_HEIGHT };
            Vector2 origin = {0, 0};
            DrawTexturePro(textures[TEXTURE_TILEMAP], source, dest, origin, 0.0f, WHITE);

        }
    }

}

void Game_Shutdown() {

    for(int i = 0; i < MAX_TEXTURES; i++) {
        UnloadTexture(textures[i]);
    }

    CloseAudioDevice();

}

const int screenWidth = 800;
const int screenHeight = 600;

int main(void)
{
    InitWindow(screenWidth, screenHeight, "Farm_Sim");
    SetTargetFPS(60);


    Game_Startup();

    while (!WindowShouldClose()) {
        
        Game_Update();

        BeginDrawing();
        ClearBackground(GRAY);

        Game_Render();

        EndDrawing();

    }
    
    Game_Shutdown();

    CloseWindow();
    return 0;
}