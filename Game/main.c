#include "raylib.h"
#include "raymath.h"

#define TILE_HEIGHT 16
#define TILE_WIDTH 16          

void GameStartup();
void GameUpdate();
void GameRender();
void GameShutdown();
void DrawTile(int pos_x, int pos_y, int texture_index_x, int texture_index_y );
void DrawWorld();
Color LerpColor(Color a, Color b, float t);

const int screenWidth = 800;
const int screenHeight = 600;

#define MAX_TEXTURES 1

typedef enum{
    TEXTURE_TILEMAP = 0
} Texture_asset;

Texture2D textures[MAX_TEXTURES];


#define WORLD_WIDTH 20 // 20 * TILE_WIDTH
#define WORLD_HEIGHT 20 // 20 * TILE_HEIGHT

// TILE TYPES
typedef enum {
    TILE_TYPE_DIRT = 0,
    TILE_TYPE_GRASS,
    TILE_TYPE_PEBBLE_1,
    TILE_TYPE_PEBBLE_2,
    TILE_TYPE_TILLED_SOIL,
    TILE_TYPE_WATERED_SOIL,
    TILE_TYPE_SEED_PLANTED,
    TILE_TYPE_CROP_GROWING,
    TILE_TYPE_CROP_READY,
} tile_type;


typedef struct {
    int x;
    int y;
    int type;
    float growthTime;
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
    ITEM_SEED,
    ITEM_WHEAT,
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

//Time const

typedef struct {
    float dayTime;     
    float timeSpeed;     
    bool isNight;
} TimeSystem;

TimeSystem worldTime = {
    .dayTime = 12.0f,   
    .timeSpeed = 0.05f, 
    .isNight = false
};


#define DAY_COLOR     (Color){ 255, 255, 255, 255 }   
#define NIGHT_COLOR   (Color){ 50, 50, 100, 255 }    
#define DUSK_COLOR    (Color){ 200, 150, 100, 255 }   
#define DAWN_COLOR    (Color){ 150, 150, 200, 255 }  
Color currentLightColor = DAY_COLOR;
RenderTexture2D lightTexture;







void Game_Startup() {

    InitAudioDevice();

    lightTexture = LoadRenderTexture(screenWidth, screenHeight);

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
    inventory.slots[1] = (InventorySlot){ITEM_WATERING_CAN, 1};
    inventory.slots[2] = (InventorySlot){ITEM_SEED, 5};
}







void Game_Update() {

    // Time of day
    worldTime.dayTime += worldTime.timeSpeed * GetFrameTime();
    if (worldTime.dayTime >= 24.0f) worldTime.dayTime = 0.0f;

    worldTime.isNight = (worldTime.dayTime > 20.0f || worldTime.dayTime < 6.0f);

    // AGRICULTURE

    // Growth time
    for (int i = 0; i < WORLD_WIDTH; i++) {
        for (int j = 0; j < WORLD_HEIGHT; j++) {
            if (world[i][j].type == TILE_TYPE_SEED_PLANTED || world[i][j].type == TILE_TYPE_CROP_GROWING) {
                
                world[i][j].growthTime += 0.1f * GetFrameTime();

                if (world[i][j].growthTime >= 0.5f && world[i][j].type == TILE_TYPE_SEED_PLANTED) {
                    world[i][j].type = TILE_TYPE_CROP_GROWING;
                }
                else if (world[i][j].growthTime >= 1.0f) {
                    world[i][j].type = TILE_TYPE_CROP_READY;
                }
            }
        }
    }

    // Harvesting
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
        int tileX = (int)(mousePos.x / TILE_WIDTH);
        int tileY = (int)(mousePos.y / TILE_HEIGHT);
    
        if (tileX >= 0 && tileX < WORLD_WIDTH && tileY >= 0 && tileY < WORLD_HEIGHT) {
            // Harvest ready crops
            if (world[tileX][tileY].type == TILE_TYPE_CROP_READY) {
                world[tileX][tileY].type = TILE_TYPE_DIRT; // Reset to dirt
                world[tileX][tileY].growthTime = 0.0f;
                
                // Add harvested item to inventory (e.g., wheat)
                for (int i = 0; i < INVENTORY_SLOTS; i++) {
                    if (inventory.slots[i].type == ITEM_WHEAT) {
                        inventory.slots[i].count++;
                        break;
                    }
                }
            }
        }
    }

    // Light color based on time

    if (worldTime.dayTime >= 4.0f && worldTime.dayTime <= 6.0f) {
        // Dawn transition (4-6)
        float factor = (worldTime.dayTime - 4.0f) / 2.0f;
        currentLightColor = LerpColor(NIGHT_COLOR, DAY_COLOR, factor);
    }
    else if (worldTime.dayTime >= 17.0f && worldTime.dayTime <= 19.0f) {
        // Dusk transition (17-19)
        float factor = (worldTime.dayTime - 17.0f) / 2.0f;
        currentLightColor = LerpColor(DAY_COLOR, NIGHT_COLOR, factor);
    }
    else if (worldTime.isNight) {
        currentLightColor = NIGHT_COLOR;
    }
    else {
        currentLightColor = DAY_COLOR;
    }

    // Time Control
    if (IsKeyPressed(KEY_MINUS)) worldTime.timeSpeed *= 0.2f;
    if (IsKeyPressed(KEY_EQUAL)) worldTime.timeSpeed *= 5.0f;

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

    // Items interaction

    // HOE TOOL
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (inventory.slots[inventory.selectedHotbarSlot].type == ITEM_HOE) {
            
            Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
            int tileX = (int)(mousePos.x / TILE_WIDTH);
            int tileY = (int)(mousePos.y / TILE_HEIGHT);
            
            if (tileX >= 0 && tileX < WORLD_WIDTH && tileY >= 0 && tileY < WORLD_HEIGHT) {
                if (world[tileX][tileY].type == TILE_TYPE_DIRT || world[tileX][tileY].type == TILE_TYPE_GRASS) {
                    world[tileX][tileY].type = TILE_TYPE_TILLED_SOIL;
                }
            }
        }
    }

    // WATERING CAN
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (inventory.slots[inventory.selectedHotbarSlot].type == ITEM_WATERING_CAN) {

            Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
            int tileX = (int)(mousePos.x / TILE_WIDTH);
            int tileY = (int)(mousePos.y / TILE_HEIGHT);
    
            if (tileX >= 0 && tileX < WORLD_WIDTH && tileY >= 0 && tileY < WORLD_HEIGHT) {
                if (world[tileX][tileY].type == TILE_TYPE_TILLED_SOIL) {
                    world[tileX][tileY].type = TILE_TYPE_WATERED_SOIL;
                }
            }
        }
    }

    // SEEDS
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (inventory.slots[inventory.selectedHotbarSlot].type == ITEM_SEED) {

            Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
            int tileX = (int)(mousePos.x / TILE_WIDTH);
            int tileY = (int)(mousePos.y / TILE_HEIGHT);
    
            if (tileX >= 0 && tileX < WORLD_WIDTH && tileY >= 0 && tileY < WORLD_HEIGHT) {
                if (world[tileX][tileY].type == TILE_TYPE_WATERED_SOIL && inventory.slots[inventory.selectedHotbarSlot].count > 0) {
                    world[tileX][tileY].type = TILE_TYPE_SEED_PLANTED;
                    inventory.slots[inventory.selectedHotbarSlot].count--;
                }
            }
        }
    }

}







void Game_Render() {

    BeginDrawing();

        BeginMode2D(camera); 

            DrawWorld();

            // Visual feedback for hoe tool
            if (inventory.slots[inventory.selectedHotbarSlot].type == ITEM_HOE) {
                Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
                int tileX = (int)(mousePos.x / TILE_WIDTH);
                int tileY = (int)(mousePos.y / TILE_HEIGHT);
            
                if (tileX >= 0 && tileX < WORLD_WIDTH && tileY >= 0 && tileY < WORLD_HEIGHT) {
                    if (world[tileX][tileY].type == TILE_TYPE_DIRT || world[tileX][tileY].type == TILE_TYPE_GRASS) {
                        DrawRectangle(tileX * TILE_WIDTH, tileY * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT, Fade(GREEN, 0.3f));
                    }
                }
            }
            
            // Visual feedback for watering can
            if (inventory.slots[inventory.selectedHotbarSlot].type == ITEM_WATERING_CAN) {
                Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
                int tileX = (int)(mousePos.x / TILE_WIDTH);
                int tileY = (int)(mousePos.y / TILE_HEIGHT);
            
                if (tileX >= 0 && tileX < WORLD_WIDTH && tileY >= 0 && tileY < WORLD_HEIGHT) {
                    if (world[tileX][tileY].type == TILE_TYPE_TILLED_SOIL) {
                        DrawRectangle(tileX * TILE_WIDTH, tileY * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT, Fade(BLUE, 0.3f));
                    }
                }
            }

            // Visual feedback for planting
            if (inventory.slots[inventory.selectedHotbarSlot].type == ITEM_SEED) {
                Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
                int tileX = (int)(mousePos.x / TILE_WIDTH);
                int tileY = (int)(mousePos.y / TILE_HEIGHT);
            
                if (tileX >= 0 && tileX < WORLD_WIDTH && tileY >= 0 && tileY < WORLD_HEIGHT) {
                    if (world[tileX][tileY].type == TILE_TYPE_WATERED_SOIL) {
                        DrawRectangle(tileX * TILE_WIDTH, tileY * TILE_HEIGHT, TILE_WIDTH, TILE_HEIGHT, Fade(YELLOW, 0.3f));
                    }
                }
            }

        EndMode2D();

        // Render light on texture
        BeginTextureMode(lightTexture);
            ClearBackground(BLANK);
            
            if (worldTime.isNight) {
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
            }
            
            DrawRectangleGradientV(0, 0, screenWidth, screenHeight/2, Fade(currentLightColor, 0.3f), BLANK);
        EndTextureMode();

        DrawTextureRec(lightTexture.texture, (Rectangle){0, 0, screenWidth, -screenHeight}, (Vector2){0, 0}, WHITE);
        
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
        // Draw Time Displey
        const char* timeOfDay = "Midday";
        if (worldTime.isNight) timeOfDay = "Night";
        else if (worldTime.dayTime >= 18.0f) timeOfDay = "Evening";
        else if (worldTime.dayTime <= 6.0f) timeOfDay = "Morning";
        else if (worldTime.dayTime >= 12.0f) timeOfDay = "Afternoon";

        DrawText(timeOfDay, 15, 90, 14, BLACK);
        DrawText(TextFormat("Time: %02d:%02d", (int)worldTime.dayTime, (int)((worldTime.dayTime - (int)worldTime.dayTime) * 60)), 15, 70, 14, BLACK);

    EndDrawing();
}






void Game_Shutdown() {

    for(int i = 0; i < MAX_TEXTURES; i++) {
        UnloadTexture(textures[i]);
    }

    CloseAudioDevice();

    UnloadRenderTexture(lightTexture);
}





// Helpig functions

void DrawWorld() {
    for(int i = 0; i < WORLD_WIDTH; i++) {
        for(int j = 0; j < WORLD_HEIGHT; j++) {
            sTile tile = world[i][j];
            int tx = 0, ty = 0;
            
            switch(tile.type) {
                case TILE_TYPE_GRASS: tx = 2; break;
                case TILE_TYPE_PEBBLE_1: tx = 1; break;
                case TILE_TYPE_PEBBLE_2: tx = 4; ty = 1; break;
                case TILE_TYPE_TILLED_SOIL: tx = 3; break;
                case TILE_TYPE_WATERED_SOIL: tx = 4; break;
                case TILE_TYPE_SEED_PLANTED: tx = 0; ty = 1; break; 
                case TILE_TYPE_CROP_GROWING: tx = 1; ty = 1; break; 
                case TILE_TYPE_CROP_READY: tx = 2; ty = 1; break;
            }
            
            DrawTile(tile.x * TILE_WIDTH, tile.y * TILE_HEIGHT, tx, ty);
        }
    }
    DrawTile(player.position.x, player.position.y, 6, 7);
}

void DrawTile(int pos_x, int pos_y, int texture_index_x, int texture_index_y ){

    Rectangle source = { (float)texture_index_x * TILE_WIDTH, (float)texture_index_y * TILE_HEIGHT, (float)TILE_WIDTH, (float)TILE_HEIGHT }; 
    Rectangle dest = { (float)(pos_x), (float)(pos_y), (float)TILE_WIDTH, (float)TILE_HEIGHT };
    DrawTexturePro(textures[TEXTURE_TILEMAP], source, dest, (Vector2){0}, 0.0f, WHITE);
}

Color LerpColor(Color a, Color b, float t) {
    t = Clamp(t, 0.0f, 1.0f); 
    float smothT = t * t * (3.0f - 2.0f * t);
    return (Color){ (unsigned char)(a.r + (b.r - a.r) * smothT), (unsigned char)(a.g + (b.g - a.g) * smothT), (unsigned char)(a.b + (b.b - a.b) * smothT), 255};
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