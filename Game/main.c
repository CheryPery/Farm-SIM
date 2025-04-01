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

typedef enum {
    TILE_TYPE_DIRT = 0,
    TILE_TYPE_GRASS,
    TILE_TYPE_PEBBLE_1,
    TILE_TYPE_PEBBLE_2,
} tile_type;


typedef struct {
    int x;
    int y;
    int type;
} sTile;

sTile world[WORLD_WIDTH][WORLD_HEIGHT];

Camera2D camera = {0};
bool zoomControlEnabled = false;

typedef struct{
    Vector2 position;
    Vector2 velocity;
    float acceleration;
    float deceleration;
} sEntity;

sEntity player;

// Inventory + Hotbar const

#define INVENTORY_SLOTS 10    
#define HOTBAR_SLOTS 3     
#define ITEM_NONE 0 

typedef enum {
    ITEM_HOE = 1,
    ITEM_WATERING_CAN,
    ITEM_SEED
} ItemType;

typedef struct {
    ItemType type;
    int count;
} InventorySlot;

typedef struct {
    InventorySlot slots[INVENTORY_SLOTS];
    int selectedHotbarSlot;
} InventorySystem;

InventorySystem inventory;
bool inventoryOpen = false;





void Game_Startup() {

    InitAudioDevice();

    // Set texture on grid
    Image image = LoadImage("assets/tilemap_packed.png");
    textures[TEXTURE_TILEMAP] = LoadTextureFromImage(image);
    UnloadImage(image);

    for(int i = 0; i < WORLD_WIDTH; i++) {
        for(int j = 0; j < WORLD_HEIGHT; j++) {
            world[i][j] = (sTile)
            {
                .x = i,
                .y = j,
                .type = GetRandomValue(TILE_TYPE_DIRT, TILE_TYPE_PEBBLE_2),
            };
        }
    }

    // Initial player position

    player = (sEntity) {
        .position = {TILE_WIDTH * 3, TILE_HEIGHT * 3},
        .velocity = {0, 0},
        .acceleration = 10.0f,
        .deceleration = 15.0f
    };

    // Camera consts

    camera.target = player.position;
    camera.offset = (Vector2){(float)screenWidth / 2, (float)screenHeight / 2};
    camera.rotation = 0.0f;
    camera.zoom = 3.0f;

    // Initial inventory

    inventory.selectedHotbarSlot = 0;

    for (int  i = 0; i < INVENTORY_SLOTS; i++) {
        inventory.slots[i].type = ITEM_NONE;
        inventory.slots[i].count = 0;
    }
    
    // Item demo
    inventory.slots[0] = (InventorySlot){ITEM_HOE, 1};
}







void Game_Update() {

    // Movement

    Vector2 inputDir = {0};

    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
    inputDir.x -= 1.0f;
    }

    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
    inputDir.x += 1.0f;
    }

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
    inputDir.y -= 1.0f;
    }

    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
    inputDir.y += 1.0f;
    }

    if (Vector2Length(inputDir) > 0) {
        inputDir = Vector2Normalize(inputDir);
    }

    float speed = 100.0f; 
    Vector2 velocity = Vector2Scale(inputDir, speed * GetFrameTime());
    Vector2 desiredVelocity = Vector2Scale(inputDir, speed);

    // Movement acceleration

    player.velocity.x = Lerp(player.velocity.x, desiredVelocity.x, 10.0f * GetFrameTime());
    player.velocity.y = Lerp(player.velocity.y, desiredVelocity.y, 10.0f * GetFrameTime());


    if (Vector2Length(inputDir) == 0) {
        player.velocity.x = Lerp(player.velocity.x, 0, 15.0f * GetFrameTime());
        player.velocity.y = Lerp(player.velocity.y, 0, 15.0f * GetFrameTime());
    }

    player.position.x += player.velocity.x * GetFrameTime();
    player.position.y += player.velocity.y * GetFrameTime();

    // Map Bundary
    player.position.x = Clamp(player.position.x, 0, WORLD_WIDTH * TILE_WIDTH - TILE_WIDTH);
    player.position.y = Clamp(player.position.y, 0, WORLD_HEIGHT * TILE_HEIGHT - TILE_HEIGHT);


    // Camera 
    float cameraLerpSpeed = 5.0f * GetFrameTime();

    camera.target.x = Lerp(camera.target.x, player.position.x, cameraLerpSpeed);
    camera.target.y = Lerp(camera.target.y, player.position.y, cameraLerpSpeed);

    // Zoom-ul 
    float wheel = GetMouseWheelMove();
    
    zoomControlEnabled = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);

    if (zoomControlEnabled) {
        if (wheel != 0) {
            camera.zoom = Clamp(camera.zoom + wheel * 0.125f, 3.0f, 8.0f);
        }
    }
    
    // Inventory mousewheel movement
    if (!zoomControlEnabled) {
        if (wheel != 0) {
            inventory.selectedHotbarSlot -= (int)wheel;
            inventory.selectedHotbarSlot = Clamp(inventory.selectedHotbarSlot, 0, HOTBAR_SLOTS-1);
        }
    }

    // Inventory open
    if (IsKeyPressed(KEY_E)) {
        inventoryOpen = !inventoryOpen;
    }

}







void Game_Render() {

    BeginMode2D(camera); 

    sTile tile;
    int texture_index_x = 0;
    int texture_index_y = 0;

    // Draw map

    for(int i = 0; i < WORLD_WIDTH; i++) {
        for(int j = 0; j < WORLD_HEIGHT; j++) {
            tile = world[i][j];
            switch (tile.type){

            case TILE_TYPE_DIRT:
                texture_index_x = 0;
                texture_index_y = 0;
                break;
            
            case TILE_TYPE_GRASS:
                texture_index_x = 2;
                texture_index_y = 0;
                break;

            case TILE_TYPE_PEBBLE_1:
                texture_index_x = 1;
                texture_index_y = 0;
                break;

            case TILE_TYPE_PEBBLE_2:
                texture_index_x = 4;
                texture_index_y = 1;
                break;

            }

            DrawTile(tile.x * TILE_WIDTH, tile.y * TILE_HEIGHT, texture_index_x, texture_index_y);

        }
    }

    // Draw player

    DrawTile(player.position.x, player.position.y, 6, 7);


    EndMode2D();

    // Draw Stats UI
    DrawRectangle(5, 5, 330, 120, Fade(LIGHTGRAY, 0.5f));
    DrawRectangleLines(5, 5, 330, 120, BLACK);

    DrawText(TextFormat("Camera Target: (%06.2f, %06.f)", camera.target.x, camera.target.y), 15, 10, 14, BLACK); 
    DrawText(TextFormat("Player Velocity: (%06.2f, %06.2f)", player.velocity.x, player.velocity.y), 15, 50, 14, BLACK);
    DrawText(TextFormat("Camera Zoom: %06.2f", camera.zoom), 15, 30, 14, BLACK);

    // Draw Hotbar

    int hotbarX = screenWidth / 2 - (HOTBAR_SLOTS * 50) / 2; 
    int hotbarY = screenHeight - 50;

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        Color slotColor = (i == inventory.selectedHotbarSlot) ? YELLOW : LIGHTGRAY;
        DrawRectangle(hotbarX + i*50, hotbarY, 40, 40, slotColor);
        if (inventory.slots[i].type != ITEM_NONE) {
            DrawText(TextFormat("%d", inventory.slots[i].count), hotbarX + i*50 + 25, hotbarY + 25, 10, BLACK);
        }
    }

    // Draw Inventory

    if (inventoryOpen) {
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            int row = i / 5;
            int col = i % 5;
            DrawRectangle(100 + col*50, 100 + row*50, 40, 40, LIGHTGRAY);

        }
    }
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