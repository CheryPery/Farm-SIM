#include "raylib.h"
#include "raymath.h"

#define TILE_HEIGHT 16
#define TILE_WIDTH 16



void GameStartup();
void GameUpdate();
void GameRender();
void GameShutdown();
void DrawTile(int pos_x, int pos_y, int texture_index_x, int texture_index_y );

const int screenWidth = 800;
const int screenHeight = 600;

#define MAX_TEXTURES 1

typedef enum{
    TEXTURE_TILEMAP = 0
} Texture_asset;

Texture2D textures[MAX_TEXTURES];


#define WORLD_WIDTH 20 // 20 * TILE_WIDTH
#define WORLD_HEIGHT 20 // 20 * TILE_HEIGHT


typedef struct {
    int x;
    int y;
} sTile;

sTile world[WORLD_WIDTH][WORLD_HEIGHT];

Camera2D camera = {0};

typedef struct{
    int x;
    int y;
} sEntity;

sEntity player;




void Game_Startup() {

    InitAudioDevice();

    // Seteza texturele pe grid

    Image image = LoadImage("assets/tilemap_packed.png");
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

    // Pozitia initiala a personajului

    player = (sEntity)
    {
        .x = TILE_WIDTH * 3,
        .y = TILE_HEIGHT * 3
    };

    // Constantele necesare pentru camera

    camera.target = (Vector2){player.x, player.y};
    camera.offset = (Vector2){(float)screenWidth / 2, (float)screenHeight / 2};
    camera.rotation = 0.0f;
    camera.zoom = 3.0f;
}







void Game_Update() {

    float x = player.x;
    float y = player.y;

    // Movement

    if(IsKeyPressed(KEY_A)) {
        x -= 1 * TILE_WIDTH;
    }
    else if(IsKeyPressed(KEY_D)) {
        x += 1 * TILE_WIDTH;
    }
    else if(IsKeyPressed(KEY_W)) {
        y -= 1 * TILE_HEIGHT;
    }
    else if(IsKeyPressed(KEY_S)) {
        y += 1 * TILE_HEIGHT;
    }

    // ZOOM-UL PENTRU CAMERA

    float wheel = GetMouseWheelMove();
    if(wheel != 0) {

        const float zoomIncrement = 0.125f;
        camera.zoom += (wheel * zoomIncrement);
        if (camera.zoom < 3.0f) camera.zoom = 3.0f;
        if (camera.zoom > 8.0f) camera.zoom = 8.0f;
    }

    player.x = x;
    player.y = y;

    camera.target = (Vector2){ player.x, player.y};

}







void Game_Render() {

    BeginMode2D(camera); 

    sTile tile;
    int texture_index_x = 0;
    int texture_index_y = 0;

    // Deseneaza un tile din intregul png

    for(int i = 0; i < WORLD_WIDTH; i++) {
        for(int j = 0; j < WORLD_HEIGHT; j++) {
            tile = world[i][j];
            texture_index_x = 1; 
            texture_index_y = 0;

            DrawTile(tile.x * TILE_WIDTH, tile.y * TILE_HEIGHT, texture_index_x, texture_index_y);

        }
    }

    // Deseneaza jucatorul

    DrawTile(camera.target.x, camera.target.y, 6, 7);
    

    EndMode2D();

    // Stats UI
    DrawRectangle(5, 5, 330, 120, Fade(LIGHTGRAY, 0.5f));
    DrawRectangleLines(5, 5, 330, 120, BLACK);

    DrawText(TextFormat("Camera Target: (%06.2f, %06.f)", camera.target.x, camera.target.y), 15, 10, 14, BLACK); 
    DrawText(TextFormat("Camera Zoom: %06.2f", camera.zoom), 15, 30, 14, BLACK);
}






void Game_Shutdown() {

    for(int i = 0; i < MAX_TEXTURES; i++) {
        UnloadTexture(textures[i]);
    }

    CloseAudioDevice();

}

void DrawTile(int pos_x, int pos_y, int texture_index_x, int texture_index_y ){

    Rectangle source = { (float)texture_index_x * TILE_WIDTH, (float)texture_index_y * TILE_HEIGHT, (float)TILE_WIDTH, (float)TILE_HEIGHT }; 
    Rectangle dest = { (float)(pos_x), (float)(pos_y), (float)TILE_WIDTH, (float)TILE_HEIGHT };
    Vector2 origin = {0, 0};
    DrawTexturePro(textures[TEXTURE_TILEMAP], source, dest, origin, 0.0f, WHITE);


}




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