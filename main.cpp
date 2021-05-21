#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include <SDL_mixer.h>

#include "Util.h"
#include "Entity.h"
#include <time.h>
#include <random>
#include <chrono>
#include <thread>
#include <vector>


#define BACKGROUND1_COUNT 3
#define BACKGROUND2_COUNT 2
#define ASTEROID_COUNT 50
#define METEOR_COUNT 20
#define PLAYER_MAX_BULLET 30
#define UFO_MAX_BULLET 21
#define WALL_COUNT 2
#define UFO_COUNT 20
#define KAMIKAZE_COUNT 30
#define CLEVER_COUNT 20
#define COIN_COUNT 100
#define DIAMOND_COUNT 20
#define MAGNET_COUNT 20
#define POWERUP_COUNT 20

using namespace std;
using namespace chrono;

struct GameState {
    Entity* player;
    Entity* ufos;
    Entity* kamikazes;
    Entity* clevers;
    Entity* playerBullets;
    Entity* ufoBullets;
    Entity* asteroids;
    Entity* meteors;
    Entity* backgrounds1;
    Entity* backgrounds2;
    Entity* camera;
    Entity* walls;
    Entity* coins;
    Entity* diamonds;
    Entity* armor;
    Entity* score;
    Entity* magnets;
    Entity* powerups;
};

// Game variables
GameState state;
enum class GameMode { MAIN_MENU, GAME_LEVEL, GAME_OVER, GAME_WIN };
GameMode mode;

//Music
Mix_Music* music;
Mix_Chunk* shoot;
Mix_Chunk* hit_ship;
Mix_Chunk* hit_enemy;
Mix_Chunk* dead;
Mix_Chunk* coin_drop;
Mix_Chunk* coin;
Mix_Chunk* game_over;
Mix_Chunk* powerup;

SDL_Window* displayWindow;
bool gameIsRunning = true;

//Start of program time
steady_clock::time_point programStart;


// Asteroid generation variables
steady_clock::time_point asteroidStart;
int nextAsteroid;

// Meteor generation variables
steady_clock::time_point meteorStart;
int nextMeteor;

// Ufo generation variables
steady_clock::time_point ufoStart;
int nextUfo;

// Kamikaze generation variables
steady_clock::time_point kamikazeStart;
int nextKamikaze;

// Clever generation variables
steady_clock::time_point cleverStart;
int nextClever;

// Mob Generation Period Variables
float asteroidPeriod;
float meteorPeriod;
float ufoPeriod;
float kamikazePeriod;
float cleverPeriod;

// Index variables
int nextPlayerBullet;
int nextCoin;

// Ship upgrades variables
int upgrade;
string aircraft;
string aircraft_hit;
float bulletVelocity;
float bulletPeriod;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;
GLuint fontTextureID;

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    displayWindow = SDL_CreateWindow("Space Infinity", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 960, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(0, 0, 640, 960);
    
    program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");
    
    viewMatrix = glm::mat4(1.0f);
    modelMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -7.0f, 7.0f, -1.0f, 1.0f);
    
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    
    glUseProgram(program.programID);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
   
    // Initialize Game Objects
    
    // Initialize Player
    state.player = new Entity();
    state.player->entityType = EntityType::SHIP;
    state.player->position = glm::vec3(0, -7, 0);
    state.player->hp = 4;
    state.player->speed = 5.0f;
    state.player->velocity.y = 2.0f;
    state.player->height = 1.05f;
    state.player->width = 0.68f;
    state.player->nextCoin = 0;
    state.player->nextDiamond = 0;
    state.player->nextMagnet = 0;
    state.player->nextPowerup = 0;
    state.player->iframe = false;
    state.player->textureID = Util::LoadTexture("Aircraft_03.png");

    // Initializing Armor
    state.armor = new Entity();
    state.armor->entityType = EntityType::ARMOR;
    state.armor->textureID = Util::LoadTexture("armor.png");
    state.armor->animRight = new int[5]{4, 3, 2, 1, 0};
    state.armor->animIndices = state.armor->animRight;
    state.armor->animFrames = 5;
    state.armor->animIndex = 0;
    state.armor->animTime = 0;
    state.armor->animCols = 5;
    state.armor->animRows = 1;

    // Initializing Magnet
    state.magnets = new Entity[MAGNET_COUNT+1];
    for (int i = 0; i < MAGNET_COUNT; i++) {
        state.magnets[i].entityType = EntityType::COIN;
        state.magnets[i].textureID = Util::LoadTexture("magnet.png");
        state.magnets[i].active = false;
        state.magnets[i].acceleration = glm::vec3(0, -4.5, 0);
        state.magnets[i].coinDropSound = false;
        state.magnets[i].coinSound = false;
        state.magnets[i].coinType = CoinType::MAGNET;
    }

    state.magnets[MAGNET_COUNT].entityType = EntityType::COIN;
    state.magnets[MAGNET_COUNT].textureID = Util::LoadTexture("magnet.png");
    state.magnets[MAGNET_COUNT].height = 0.5f;
    state.magnets[MAGNET_COUNT].width = 0.5f;
    state.magnets[MAGNET_COUNT].active = false;
    state.magnets[MAGNET_COUNT].speed = 1.0f;
    state.magnets[MAGNET_COUNT].coinDropSound = false;
    state.magnets[MAGNET_COUNT].coinSound = false;
    state.magnets[MAGNET_COUNT].coinType = CoinType::ICON;

    // Initializing Powerups
    state.powerups = new Entity[POWERUP_COUNT];
    for (int i = 0; i < POWERUP_COUNT; i++) {
        state.powerups[i].entityType = EntityType::COIN;
        state.powerups[i].textureID = Util::LoadTexture("powerup_armor.png");
        state.powerups[i].active = false;
        state.powerups[i].acceleration = glm::vec3(0, -4.5, 0);
        state.powerups[i].coinDropSound = false;
        state.powerups[i].coinSound = false;
        state.powerups[i].coinType = CoinType::POWERUP;
    }


    // Initializing score
    state.score = new Entity();
    state.score->entityType = EntityType::SCORE;
    state.score->textureID = Util::LoadTexture("score.png");
    state.score->animRight = new int[1]{ 0 };
    state.score->animIndices = state.score->animRight;
    state.score->animFrames = 1;
    state.score->animIndex = 0;
    state.score->animTime = 0;
    state.score->animCols = 1;
    state.score->animRows = 1;

    // Initialize Ufo
    state.ufos = new Entity[UFO_COUNT];
    nextUfo = 0;
    for (int i = 0; i < UFO_COUNT; i++) {
        state.ufos[i].entityType = EntityType::ENEMY;
        state.ufos[i].aiType = AIType::UFO;
        state.ufos[i].height = 1.0f;
        state.ufos[i].width = 1.0f;
        state.ufos[i].movement.x = 1.0f;
        state.ufos[i].textureID = Util::LoadTexture("ufo.png");
        state.ufos[i].ufoBulletStart = steady_clock::now();
        state.ufos[i].nextUfoBullet = 0;
        state.ufos[i].hp = 4;
        state.ufos[i].active = false;
    }

    // Initialize Kamikaze
    nextKamikaze = 0;
    state.kamikazes = new Entity[KAMIKAZE_COUNT];
    for (int i = 0; i < KAMIKAZE_COUNT; i++) {
        state.kamikazes[i].entityType = EntityType::ENEMY;
        state.kamikazes[i].aiType = AIType::KAMIKAZE;
        state.kamikazes[i].height = 0.82f;
        state.kamikazes[i].width = 0.82f;
        state.kamikazes[i].movement.x = 1.0f;
        state.kamikazes[i].textureID = Util::LoadTexture("enemy_kamikaze.png");
        state.kamikazes[i].hp = 1;
        state.kamikazes[i].active = false;
    }

    // Initialize Clever
    nextClever = 0;
    state.clevers = new Entity[CLEVER_COUNT];
    for (int i = 0; i < CLEVER_COUNT; i++) {
        state.clevers[i].entityType = EntityType::ENEMY;
        state.clevers[i].aiType = AIType::CLEVER;
        state.clevers[i].height = 2.0f;
        state.clevers[i].width = 2.0f;
        state.clevers[i].movement.x = -1.0f;
        state.clevers[i].textureID = Util::LoadTexture("enemy_clever.png");
        state.clevers[i].cleverAttackStart = steady_clock::now();
        state.clevers[i].hp = 10;
        state.clevers[i].active = false;
    }

    // Initialize Camera for Main Menu
    state.camera = new Entity();
    state.camera->position = glm::vec3(0, 0, 0);
    state.camera->velocity.y = 2.0f;


    // Initialize Player Bullets
    nextPlayerBullet = 0;
    state.playerBullets = new Entity[PLAYER_MAX_BULLET];
    for (int i = 0; i < PLAYER_MAX_BULLET; i++) {
        state.playerBullets[i].entityType = EntityType::PLAYERBULLET;
        state.playerBullets[i].textureID = Util::LoadTexture("bullet_blue0000.png");
        state.playerBullets[i].height = 0.3f;
        state.playerBullets[i].width = 0.3f;
        state.playerBullets[i].velocity.y = 13.0f;
        state.playerBullets[i].active = false;
    }

    // Initializing Ufo Bullets
    state.ufoBullets = new Entity[UFO_MAX_BULLET];
    for (int i = 0; i < UFO_MAX_BULLET; i++) {
        state.ufoBullets[i].entityType = EntityType::UFOBULLET;
        state.ufoBullets[i].textureID = Util::LoadTexture("laser_enemy.png");
        state.ufoBullets[i].height = 0.3f;
        state.ufoBullets[i].width = 0.2f;
        state.ufoBullets[i].speed = 1.0f;
        state.ufoBullets[i].active = false;
    }

    // Initialize Walls
    state.walls = new Entity[WALL_COUNT];
    for (int i = 0; i < WALL_COUNT; i++) {
        state.walls[i].entityType = EntityType::WALL;
        state.walls[i].height = 14.0f;
        state.walls[i].width = 0.0f;
    }
    

    // Initialize Platform 
    state.asteroids = new Entity[ASTEROID_COUNT];
    for (int i = 0; i < ASTEROID_COUNT; i++) {
        state.asteroids[i].entityType = EntityType::ASTEROID;
        state.asteroids[i].textureID = Util::LoadTexture("Asteroid Brown.png");
        state.asteroids[i].height = 0.6f;
        state.asteroids[i].width = 0.8f;
        state.asteroids[i].velocity.y = -1.0f;
        state.asteroids[i].active = false;
        state.asteroids[i].hp = 2;
    }

    // Initialize Meteors
    state.meteors = new Entity[METEOR_COUNT];
    for (int i = 0; i < METEOR_COUNT; i++) {
        state.meteors[i].entityType = EntityType::METEOR;
        state.meteors[i].textureID = Util::LoadTexture("Meteor1.png");
        state.meteors[i].spriteIndices = new int[0];
        state.meteors[i].height = 0.5f;
        state.meteors[i].width = 0.5f;
        state.meteors[i].velocity.y = -4.0f;
        state.meteors[i].active = false;
    }

    // Initialize Backgrounds
    state.backgrounds1 = new Entity[BACKGROUND1_COUNT];
    state.backgrounds1[0].entityType = EntityType::BACKGROUND;
    state.backgrounds1[0].position = glm::vec3(0);
    state.backgrounds1[0].textureID = Util::LoadTexture("n1-bottom.png");
    state.backgrounds1[1].entityType = EntityType::BACKGROUND;
    state.backgrounds1[1].position = glm::vec3(0.0f, 14.0f, 0.0);
    state.backgrounds1[1].textureID = Util::LoadTexture("n1-top.png");
    state.backgrounds1[2].entityType = EntityType::BACKGROUND;
    state.backgrounds1[2].position = glm::vec3(0.0f, -14.0f, 0.0);
    state.backgrounds1[2].textureID = Util::LoadTexture("n1-top.png");

    state.backgrounds2 = new Entity[BACKGROUND2_COUNT];
    state.backgrounds2[0].entityType = EntityType::BACKGROUND;
    state.backgrounds2[0].position = glm::vec3(0);
    state.backgrounds2[0].textureID = Util::LoadTexture("n3-bottom.png");
    state.backgrounds2[1].entityType = EntityType::BACKGROUND;
    state.backgrounds2[1].position = glm::vec3(0.0f, 14.0f, 0.0);
    state.backgrounds2[1].textureID = Util::LoadTexture("n3-top.png");

    // Initialize coins
    state.coins = new Entity[COIN_COUNT];
    for (int i = 0; i < COIN_COUNT; i++) {
        state.coins[i].entityType = EntityType::COIN;
        state.coins[i].textureID = Util::LoadTexture("coin.png");
        state.coins[i].height = 0.5f;
        state.coins[i].width = 0.5f;
        state.coins[i].velocity.y = -2.0f;
        state.coins[i].active = false;
        state.coins[i].speed = 1.0f;
        state.coins[i].acceleration = glm::vec3(0, -4.5, 0);
        state.coins[i].coinDropSound = false;
        state.coins[i].coinSound = false;
        state.coins[i].coinType = CoinType::COIN;
    }

    // Initialize diamonds
    state.diamonds = new Entity[COIN_COUNT];
    for (int i = 0; i < DIAMOND_COUNT; i++) {
        state.diamonds[i].entityType = EntityType::COIN;
        state.diamonds[i].textureID = Util::LoadTexture("diamond blue.png");
        state.diamonds[i].height = 0.5f;
        state.diamonds[i].width = 0.5f;
        state.diamonds[i].velocity.y = -2.0f;
        state.diamonds[i].active = false;
        state.diamonds[i].speed = 1.0f;
        state.diamonds[i].acceleration = glm::vec3(0, -4.5, 0);
        state.diamonds[i].coinDropSound = false;
        state.diamonds[i].coinSound = false;
        state.diamonds[i].coinType = CoinType::DIAMOND;
    }


    // Initial programTime
    programStart = chrono::steady_clock::now();
    
    // Initial asteroidTime
    asteroidStart = chrono::steady_clock::now();
    nextAsteroid = 0;

    // Initial meteorTime
    meteorStart = chrono::steady_clock::now();
    nextMeteor = 0;

    // Start Audio
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    music = Mix_LoadMUS("music.mp3");
    Mix_VolumeMusic(MIX_MAX_VOLUME / 25);
    Mix_PlayMusic(music, -1);
    shoot = Mix_LoadWAV("laser1.wav");
    Mix_VolumeChunk(shoot, MIX_MAX_VOLUME / 8);
    hit_ship = Mix_LoadWAV("hit_ship.wav");
    Mix_VolumeChunk(hit_ship, MIX_MAX_VOLUME / 2);
    hit_enemy = Mix_LoadWAV("hit_enemy.wav");
    Mix_VolumeChunk(hit_enemy, MIX_MAX_VOLUME / 8);
    dead = Mix_LoadWAV("explosion.wav");
    Mix_VolumeChunk(dead, MIX_MAX_VOLUME / 4);
    coin_drop = Mix_LoadWAV("coin_drop.wav");
    Mix_VolumeChunk(coin_drop, MIX_MAX_VOLUME / 2);
    coin = Mix_LoadWAV("coin.wav");
    Mix_VolumeChunk(coin, MIX_MAX_VOLUME / 4);
    game_over = Mix_LoadWAV("game_over.wav");
    Mix_VolumeChunk(game_over, MIX_MAX_VOLUME / 2);
    powerup = Mix_LoadWAV("powerup.wav");
    Mix_VolumeChunk(powerup, MIX_MAX_VOLUME / 2);

    // Initial Game Mode
    mode = GameMode::MAIN_MENU;
    
    // Initialize Font Texture
    fontTextureID = Util::LoadTexture("font1.png");
    
    // Initialize basic ship
    upgrade = 0;
    aircraft = "Aircraft_03.png";
    aircraft_hit = "Aircraft_03_hit.png";
    bulletVelocity = 13.0f;
    bulletPeriod = 0.6f;

    // Initialize entity generation periods
    asteroidPeriod = 0.5f;
    meteorPeriod = 0.5f;
    ufoPeriod = 15.0f;
    kamikazePeriod = 10.0f;
    cleverPeriod = 30.0f;
}

void fire() {
    if (upgrade == 0) {
        state.playerBullets[nextPlayerBullet].position = glm::vec3(state.player->position.x, state.player->position.y + 0.68, 0);
        state.playerBullets[nextPlayerBullet].active = true;
        Mix_PlayChannel(-1, shoot, 0);
        nextPlayerBullet++;
        if (nextPlayerBullet == PLAYER_MAX_BULLET) nextPlayerBullet = 0;
    }

    else if (upgrade == 1) {
        state.playerBullets[nextPlayerBullet].position = glm::vec3(state.player->position.x, state.player->position.y + 0.68, 0);
        state.playerBullets[nextPlayerBullet].active = true;
        Mix_PlayChannel(-1, shoot, 0);
        nextPlayerBullet++;
        if (nextPlayerBullet == PLAYER_MAX_BULLET) nextPlayerBullet = 0;
        state.player->textureID = Util::LoadTexture("Aircraft_04.png");
        aircraft = "Aircraft_04.png";
        aircraft_hit = "Aircraft_04_hit.png";
        for (int i = 0; i < PLAYER_MAX_BULLET; i++) {
            state.playerBullets[i].textureID = Util::LoadTexture("bullet_orange0000.png");
        }
    }

    else if (upgrade == 2) {
        state.playerBullets[nextPlayerBullet].position = glm::vec3(state.player->position.x-0.5, state.player->position.y + 0.68, 0);
        state.playerBullets[nextPlayerBullet].active = true;
        Mix_PlayChannel(-1, shoot, 0);
        nextPlayerBullet++;
        if (nextPlayerBullet == PLAYER_MAX_BULLET) nextPlayerBullet = 0;
        state.playerBullets[nextPlayerBullet].position = glm::vec3(state.player->position.x+0.5, state.player->position.y + 0.68, 0);
        state.playerBullets[nextPlayerBullet].active = true;
        Mix_PlayChannel(-1, shoot, 0);
        nextPlayerBullet++;
        if (nextPlayerBullet == PLAYER_MAX_BULLET) nextPlayerBullet = 0;
        state.player->textureID = Util::LoadTexture("Aircraft_02.png");
        aircraft = "Aircraft_02.png";
        aircraft_hit = "Aircraft_02_hit.png";
        for (int i = 0; i < PLAYER_MAX_BULLET; i++) {
            state.playerBullets[i].textureID = Util::LoadTexture("bullet_purple0000.png");
        }
    }

    else if (upgrade == 3) {
        state.playerBullets[nextPlayerBullet].position = glm::vec3(state.player->position.x - 0.6, state.player->position.y + 0.68, 0);
        state.playerBullets[nextPlayerBullet].active = true;
        Mix_PlayChannel(-1, shoot, 0);
        nextPlayerBullet++;
        if (nextPlayerBullet == PLAYER_MAX_BULLET) nextPlayerBullet = 0;
        state.playerBullets[nextPlayerBullet].position = glm::vec3(state.player->position.x, state.player->position.y + 0.68, 0);
        state.playerBullets[nextPlayerBullet].active = true;
        Mix_PlayChannel(-1, shoot, 0);
        nextPlayerBullet++;
        if (nextPlayerBullet == PLAYER_MAX_BULLET) nextPlayerBullet = 0;
        state.playerBullets[nextPlayerBullet].position = glm::vec3(state.player->position.x + 0.6, state.player->position.y + 0.68, 0);
        state.playerBullets[nextPlayerBullet].active = true;
        Mix_PlayChannel(-1, shoot, 0);
        nextPlayerBullet++;
        if (nextPlayerBullet == PLAYER_MAX_BULLET) nextPlayerBullet = 0;
        state.player->textureID = Util::LoadTexture("Aircraft_07.png");
        aircraft = "Aircraft_07.png";
        aircraft_hit = "Aircraft_07_hit.png";
        for (int i = 0; i < PLAYER_MAX_BULLET; i++) {
            state.playerBullets[i].textureID = Util::LoadTexture("laser_ship.png");
        }
    }

    else if (upgrade == 4) {
        state.playerBullets[nextPlayerBullet].position = glm::vec3(state.player->position.x - 0.6, state.player->position.y + 0.68, 0);
        state.playerBullets[nextPlayerBullet].active = true;
        Mix_PlayChannel(-1, shoot, 0);
        nextPlayerBullet++;
        if (nextPlayerBullet == PLAYER_MAX_BULLET) nextPlayerBullet = 0;
        state.playerBullets[nextPlayerBullet].position = glm::vec3(state.player->position.x - 0.4, state.player->position.y + 0.68, 0);
        state.playerBullets[nextPlayerBullet].active = true;
        Mix_PlayChannel(-1, shoot, 0);
        nextPlayerBullet++;
        if (nextPlayerBullet == PLAYER_MAX_BULLET) nextPlayerBullet = 0;
        state.playerBullets[nextPlayerBullet].position = glm::vec3(state.player->position.x + 0.4, state.player->position.y + 0.68, 0);
        state.playerBullets[nextPlayerBullet].active = true;
        Mix_PlayChannel(-1, shoot, 0);
        nextPlayerBullet++;
        if (nextPlayerBullet == PLAYER_MAX_BULLET) nextPlayerBullet = 0;
        state.playerBullets[nextPlayerBullet].position = glm::vec3(state.player->position.x + 0.6, state.player->position.y + 0.68, 0);
        state.playerBullets[nextPlayerBullet].active = true;
        Mix_PlayChannel(-1, shoot, 0);
        nextPlayerBullet++;
        if (nextPlayerBullet == PLAYER_MAX_BULLET) nextPlayerBullet = 0;
        state.player->textureID = Util::LoadTexture("Aircraft_06.png");
        aircraft = "Aircraft_06.png";
        aircraft_hit = "Aircraft_06_hit.png";
        for (int i = 0; i < PLAYER_MAX_BULLET; i++) {
            state.playerBullets[i].textureID = Util::LoadTexture("laser_ship.png");
        }
    }
}

float inputLastTicks;
float playerBulletAccumlator = 0.0f;

void ProcessInputMainMenu() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_RETURN:
                // Starts the game level
                mode = GameMode::GAME_LEVEL;
                break;
            }
            break; // SDL_KEYDOWN
        }
    }
}

void ProcessInputGameLevel() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - inputLastTicks;
    inputLastTicks = ticks;

    if (state.player->active) {
        playerBulletAccumlator += deltaTime;
        if (playerBulletAccumlator > bulletPeriod) {
            fire();
            playerBulletAccumlator = 0.0f;
        }
    }

    state.player->movement = glm::vec3(0);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            }
            break; // SDL_KEYDOWN
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_LEFT]) {
        state.player->movement.x = -1.0f;
    }
    else if (keys[SDL_SCANCODE_RIGHT]) {
        state.player->movement.x = 1.0f;
    }


    if (glm::length(state.player->movement) > 1.0f) {
        state.player->movement = glm::normalize(state.player->movement);
    }
}

void ProcessInputGameOver() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;
        }
    }
}

void ProcessInputGameWin() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;
        }
    }
}

void ProcessInput() {

    switch (mode) {
    case GameMode::MAIN_MENU:
        ProcessInputMainMenu();
        break;
    case GameMode::GAME_LEVEL:
        ProcessInputGameLevel();
        break;
    case GameMode::GAME_OVER:
        ProcessInputGameOver();
        break;
    case GameMode::GAME_WIN:
        ProcessInputGameWin();
        break;
    }
}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;

void UpdateMainMenu(float deltaTime) {
    deltaTime += accumulator;

    if (deltaTime < FIXED_TIMESTEP) {
        accumulator = deltaTime;
        return;
    }


    auto current = steady_clock::now();
    // Updating positions of the meteors randomly and when enough time has passed since the last generation of the meteors
    if (duration_cast<seconds>(current - meteorStart).count() > 0.5) {
        meteorStart = steady_clock::now();
        random_device rd;
        default_random_engine generator(rd());
        uniform_real_distribution<double> distribution(-4.5, 4.5);
        double pos = distribution(generator);
        state.meteors[nextMeteor].position = glm::vec3(pos, state.camera->position.y + 13, 0);
        state.meteors[nextMeteor].active = true;
        nextMeteor++;
        if (nextMeteor == METEOR_COUNT) nextMeteor = 0;
    }


    while (deltaTime >= FIXED_TIMESTEP) {
        // Update. Notice it's FIXED_TIMESTEP. Not deltaTime
        state.camera->Update(FIXED_TIMESTEP, NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

        for (int i = 0; i < BACKGROUND2_COUNT; i++) {
            state.backgrounds2[i].Update(0, NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
        }

        for (int i = 0; i < METEOR_COUNT; i++) {
            state.meteors[i].Update(FIXED_TIMESTEP, NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
        }

        deltaTime -= FIXED_TIMESTEP;
    }

    accumulator = deltaTime;

    viewMatrix = glm::mat4(1.0f);

    if (state.camera->position.y >= state.backgrounds2[1].position.y) {
        state.backgrounds2[0].position.y = (state.backgrounds2[1].position.y) + 14;
        if (state.camera->position.y >= state.backgrounds2[0].position.y)
            state.backgrounds2[1].position.y = (state.backgrounds2[0].position.y) + 14;
    }

    viewMatrix = glm::translate(viewMatrix, glm::vec3(0, -state.camera->position.y, 0));

}

void UpdateGameWin(float deltaTime) {
    deltaTime += accumulator;

    if (deltaTime < FIXED_TIMESTEP) {
        accumulator = deltaTime;
        return;
    }


    auto current = steady_clock::now();
    // Updating positions of the meteors randomly and when enough time has passed since the last generation of the meteors
    if (duration_cast<seconds>(current - meteorStart).count() > 0.5) {
        meteorStart = steady_clock::now();
        random_device rd;
        default_random_engine generator(rd());
        uniform_real_distribution<double> distribution(-4.5, 4.5);
        double pos = distribution(generator);
        state.meteors[nextMeteor].position = glm::vec3(pos, state.camera->position.y + 13, 0);
        state.meteors[nextMeteor].active = true;
        nextMeteor++;
        if (nextMeteor == METEOR_COUNT) nextMeteor = 0;
    }


    while (deltaTime >= FIXED_TIMESTEP) {
        // Update. Notice it's FIXED_TIMESTEP. Not deltaTime
        state.camera->Update(FIXED_TIMESTEP, NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

        for (int i = 0; i < BACKGROUND2_COUNT; i++) {
            state.backgrounds2[i].Update(0, NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
        }

        for (int i = 0; i < METEOR_COUNT; i++) {
            state.meteors[i].Update(FIXED_TIMESTEP, NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
        }

        deltaTime -= FIXED_TIMESTEP;
    }

    accumulator = deltaTime;

    viewMatrix = glm::mat4(1.0f);

    if (state.camera->position.y >= state.backgrounds2[1].position.y) {
        state.backgrounds2[0].position.y = (state.backgrounds2[1].position.y) + 14;
        if (state.camera->position.y >= state.backgrounds2[0].position.y)
            state.backgrounds2[1].position.y = (state.backgrounds2[0].position.y) + 14;
    }

    viewMatrix = glm::translate(viewMatrix, glm::vec3(0, -state.camera->position.y, 0));

}

void UpdateGameLevel(float deltaTime) {
    deltaTime += accumulator;

    if (deltaTime < FIXED_TIMESTEP) {
        accumulator = deltaTime;
        return;
    }
    auto current = steady_clock::now();

    if (state.player->score >= 50 and state.player->score < 200) { // 50 points for Upgrade 1
        upgrade = 1;
        bulletPeriod = 0.5f;
        bulletVelocity = 14.0f;
        asteroidPeriod = 0.5f;
        meteorPeriod = 0.5f;
        ufoPeriod = 14.0f;
        kamikazePeriod = 9.0f;
        cleverPeriod = 27.0f;
    }

    else if (state.player->score >= 200 and state.player->score < 500) { // 200 points for Upgrade 2
        upgrade = 2;
        bulletPeriod = 0.4f;
        bulletVelocity = 15.0f;
        asteroidPeriod = 0.4f;
        meteorPeriod = 0.4f;
        ufoPeriod = 10.0f;
        kamikazePeriod = 8.0f;
        cleverPeriod = 22.0f;
    }

    else if (state.player->score >= 500 and state.player->score < 1000) { // 500 points for Upgrade 3
        upgrade = 3;
        bulletPeriod = 0.3f;
        bulletVelocity = 16.0f;
        asteroidPeriod = 0.3f;
        meteorPeriod = 0.4f;
        ufoPeriod = 7.0f;
        kamikazePeriod = 5.0f;
        cleverPeriod = 15.0f;
    }

    else if (state.player->score >= 1337) { // 1000 points for Upgrade 4
        upgrade = 4;
        bulletPeriod = 0.25f;
        bulletVelocity = 16.0f;
        asteroidPeriod = 0.2f;
        meteorPeriod = 0.3f;
        ufoPeriod = 4.0f;
        kamikazePeriod = 3.0f;
        cleverPeriod = 10.0f;
    }

    if (state.player->score >= 2000) { // You win
        mode = GameMode::GAME_WIN;
    }

    

    if (state.player->iframe) {
        state.player->textureID = Util::LoadTexture(aircraft_hit.c_str());
        if (duration_cast<seconds>(current - state.player->iframeStart).count() >= 2.0) {
            state.player->textureID = Util::LoadTexture(aircraft.c_str());
            state.player->iframe = false;
        }
    }
    if (state.player->magnet) {
        if (duration_cast<seconds>(current - state.player->magnetStart).count() >= state.player->magnetTime) {
            state.player->magnet = false;
            state.player->magnetTime = 10.0f;
            state.magnets[MAGNET_COUNT].active = false;
        }
        else {
            state.magnets[MAGNET_COUNT].active = true;

        }
    }

    // Updating positions of the asteroids randomly and when enough time passed since last generation of asteroids
    if (duration_cast<seconds>(current - asteroidStart).count() > asteroidPeriod) {
        asteroidStart = steady_clock::now();
        random_device rd;
        default_random_engine generator(rd());
        uniform_real_distribution<double> distribution(-4.5, 4.5);
        double pos = distribution(generator);
        state.asteroids[nextAsteroid].position = glm::vec3(pos, state.player->position.y + 13, 0);
        state.asteroids[nextAsteroid].active = true;
        state.asteroids[nextAsteroid].hp = 2;
        nextAsteroid++;
        if (nextAsteroid == ASTEROID_COUNT) nextAsteroid = 0;
    }

    // Updating positions of the meteors randomly and when enough time has passed since the last generation of the meteors
    if (duration_cast<seconds>(current - meteorStart).count() > meteorPeriod) {
        meteorStart = steady_clock::now();
        random_device rd;
        default_random_engine generator(rd());
        uniform_real_distribution<double> distribution(-4.5, 4.5);
        double pos = distribution(generator);
        state.meteors[nextMeteor].position = glm::vec3(pos, state.player->position.y + 13, 0);
        state.meteors[nextMeteor].active = true;
        nextMeteor++;
        if (nextMeteor == METEOR_COUNT) nextMeteor = 0;
    }

    // Updating positions of the ufo randomly and when enough time has passed since the last generation of the meteors
    if (duration_cast<seconds>(current - ufoStart).count() > ufoPeriod) {
        ufoStart = steady_clock::now();
        state.ufos[nextUfo].position = glm::vec3(0, state.player->position.y + 15, 0);
        state.ufos[nextUfo].active = true;
        state.ufos[nextUfo].hp = 4;
        nextUfo++;
        if (nextUfo == UFO_COUNT) nextUfo = 0;
    }

    // Updating positions of the kamikaze randomly and when enough time has passed since the last generation of the meteors
    if (duration_cast<seconds>(current - kamikazeStart).count() > kamikazePeriod) {
        kamikazeStart = steady_clock::now();
        random_device rd;
        default_random_engine generator(rd());
        uniform_real_distribution<double> distribution(-4.5, 4.5);
        double pos = distribution(generator);
        state.kamikazes[nextKamikaze].position = glm::vec3(pos, state.player->position.y + 15, 0);
        state.kamikazes[nextKamikaze].active = true;
        state.kamikazes[nextKamikaze].hp = 1;
        nextKamikaze++;
        if (nextKamikaze == KAMIKAZE_COUNT) nextKamikaze = 0;
    }

    // Updating positions of the clever randomly and when enough time has passed since the last generation of the meteors
    if (duration_cast<seconds>(current - cleverStart).count() > cleverPeriod) {
        cleverStart = steady_clock::now();
        random_device rd;
        default_random_engine generator(rd());
        uniform_real_distribution<double> distribution(-4.5, 4.5);
        double pos = distribution(generator);
        state.clevers[nextClever].position = glm::vec3(pos, state.player->position.y + 15, 0);
        state.clevers[nextClever].active = true;
        state.clevers[nextClever].hp = 10;
        nextClever++;
        if (nextClever == CLEVER_COUNT) nextClever = 0;
    }



    // Changing meteor texture 


    while (deltaTime >= FIXED_TIMESTEP) {
        // Update. Notice it's FIXED_TIMESTEP. Not deltaTime

        for (int i = 0; i < BACKGROUND1_COUNT; i++) {
            state.backgrounds1[i].Update(0, NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
        }

        // Wall positions
        for (int i = 0; i < WALL_COUNT; i++) {
            if (i == 0)
                state.walls[i].position = glm::vec3(-5, state.player->position.y + 5, 0);
            else 
                state.walls[i].position = glm::vec3(5, state.player->position.y + 5, 0);
        }

        for (int i = 0; i < PLAYER_MAX_BULLET; i++) {
            state.playerBullets[i].Update(FIXED_TIMESTEP, state.player, state.asteroids, ASTEROID_COUNT, NULL, 0, NULL, 0, NULL, 0, state.ufos, UFO_COUNT, state.coins, COIN_COUNT, state.diamonds, DIAMOND_COUNT, state.magnets, MAGNET_COUNT, state.powerups, POWERUP_COUNT, state.kamikazes, KAMIKAZE_COUNT, state.clevers, CLEVER_COUNT);
            if (state.playerBullets[i].collided) {
                Mix_PlayChannel(-1, hit_enemy, 0);
                state.playerBullets[i].collided = false;
            }
        }

        for (int i = 0; i < UFO_MAX_BULLET; i++) {
            state.ufoBullets[i].Update(FIXED_TIMESTEP, NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
        }

        for (int i = 0; i < METEOR_COUNT; i++) {
            state.meteors[i].Update(FIXED_TIMESTEP, NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
        }

        for (int i = 0; i < ASTEROID_COUNT; i++) {
            state.asteroids[i].Update(FIXED_TIMESTEP, NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
        }

        for (int i = 0; i < UFO_COUNT; i++) {
            state.ufos[i].Update(FIXED_TIMESTEP, state.player, NULL, 0, NULL, 0, state.walls, WALL_COUNT, state.ufoBullets, UFO_MAX_BULLET, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
            if (state.ufos[i].collided) {
                Mix_PlayChannel(-1, dead, 0);
                state.ufos[i].collided = false;
            }
        }

        for (int i = 0; i < KAMIKAZE_COUNT; i++) {
            state.kamikazes[i].Update(FIXED_TIMESTEP, state.player, NULL, 0, NULL, 0, state.walls, WALL_COUNT, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
            if (state.kamikazes[i].collided) {
                Mix_PlayChannel(-1, dead, 0);
                state.kamikazes[i].collided = false;
            }
        }

        for (int i = 0; i < CLEVER_COUNT; i++) {
            state.clevers[i].Update(FIXED_TIMESTEP, state.player, NULL, 0, NULL, 0, state.walls, WALL_COUNT, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
            if (state.clevers[i].collided) {
                Mix_PlayChannel(-1, dead, 0);
                state.clevers[i].collided = false;
            }
        }

        for (int i = 0; i < COIN_COUNT; i++) {
            state.coins[i].Update(FIXED_TIMESTEP, state.player, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
            if (state.coins[i].coinDropSound) {
                Mix_PlayChannel(-1, coin_drop, 0);
                state.coins[i].coinDropSound = false;
            }
            if (state.coins[i].coinSound) {
                Mix_PlayChannel(-1, coin, 0);
                state.coins[i].coinSound = false;
            }
        }

        for (int i = 0; i < DIAMOND_COUNT; i++) {
            state.diamonds[i].Update(FIXED_TIMESTEP, state.player, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
            if (state.diamonds[i].coinDropSound) {
                Mix_PlayChannel(-1, coin_drop, 0);
                state.diamonds[i].coinDropSound = false;
            }
            if (state.diamonds[i].coinSound) {
                Mix_PlayChannel(-1, coin, 0);
                state.diamonds[i].coinSound = false;
            }
        }

        for (int i = 0; i < MAGNET_COUNT; i++) {
            state.magnets[i].Update(FIXED_TIMESTEP, state.player, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
            if (state.magnets[i].coinDropSound) {
                Mix_PlayChannel(-1, coin_drop, 0);
                state.magnets[i].coinDropSound = false;
            }
            if (state.magnets[i].coinSound) {
                Mix_PlayChannel(-1, coin, 0);
                state.magnets[i].coinSound = false;
            }
        }

        for (int i = 0; i < POWERUP_COUNT; i++) {
            state.powerups[i].Update(FIXED_TIMESTEP, state.player, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);
            if (state.powerups[i].coinDropSound) {
                Mix_PlayChannel(-1, coin_drop, 0);
                state.powerups[i].coinDropSound = false;
            }
            if (state.powerups[i].coinSound) {
                Mix_PlayChannel(-1, powerup, 0);
                state.powerups[i].coinSound = false;
            }
        }


        state.armor->position = glm::vec3(-3.10, state.player->position.y + 11.2, 0);
        state.armor->Update(0, state.player, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

        state.score->position = glm::vec3(3.10, state.player->position.y + 11.2, 0);
        state.score->Update(0, NULL, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

        state.magnets[MAGNET_COUNT].position = glm::vec3(-3.75, state.player->position.y + 10.50, 0);
        state.magnets[MAGNET_COUNT].Update(FIXED_TIMESTEP, state.player, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0);

        state.player->Update(FIXED_TIMESTEP, NULL, state.asteroids, ASTEROID_COUNT, state.meteors, METEOR_COUNT, state.walls, WALL_COUNT, state.ufoBullets, UFO_MAX_BULLET, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, state.kamikazes, KAMIKAZE_COUNT, state.clevers, CLEVER_COUNT);

        if (state.player->collided && state.player->hp != 0) {
            Mix_PlayChannel(-1, hit_ship, 0);
            state.player->collided = false;
        }


        deltaTime -= FIXED_TIMESTEP;
    }

    accumulator = deltaTime;

    viewMatrix = glm::mat4(1.0f);

    if (state.player->position.y + 5 >= state.backgrounds1[1].position.y) {
        state.backgrounds1[0].position.y = (state.backgrounds1[1].position.y) + 14;
        if (state.player->position.y + 5 >= state.backgrounds1[0].position.y)
            state.backgrounds1[1].position.y = (state.backgrounds1[0].position.y) + 14;
    }

    if (state.player->position.y >= -7)
        viewMatrix = glm::translate(viewMatrix, glm::vec3(0, -state.player->position.y - 5, 0));

}

void Update() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;

    switch (mode) {
    case GameMode::MAIN_MENU:
        UpdateMainMenu(deltaTime);
        break;
    case GameMode::GAME_LEVEL:
        UpdateGameLevel(deltaTime);
        break;
    case GameMode::GAME_OVER:
        break;
    case GameMode::GAME_WIN:
        UpdateGameWin(deltaTime);
        break;
    }
}

void RenderMainMenu() {

    for (int i = 0; i < BACKGROUND2_COUNT; i++) {
        state.backgrounds2[i].Render(&program);
    }

    for (int i = 0; i < METEOR_COUNT; i++) {
        state.meteors[i].Render(&program);
    }

    Util::DrawText(&program, fontTextureID, "Space 1337", 0.8f, -0.25f, glm::vec3(-2.75, state.camera->position.y, 0));
    Util::DrawText(&program, fontTextureID, "Play: [Enter]", 0.60f, -0.25f, glm::vec3(-2.2, state.camera->position.y-1, 0));

}

void RenderGameWin() {

    for (int i = 0; i < BACKGROUND2_COUNT; i++) {
        state.backgrounds2[i].Render(&program);
    }

    for (int i = 0; i < METEOR_COUNT; i++) {
        state.meteors[i].Render(&program);
    }

    Util::DrawText(&program, fontTextureID, "You win!", 0.8f, -0.25f, glm::vec3(-2.0, state.camera->position.y, 0));
    Util::DrawText(&program, fontTextureID, "Elite Player", 0.60f, -0.25f, glm::vec3(-2.0, state.camera->position.y - 1, 0));

}

void RenderGameLevel() {
    for (int i = 0; i < BACKGROUND1_COUNT; i++) {
        state.backgrounds1[i].Render(&program);
    }

    for (int i = 0; i < PLAYER_MAX_BULLET; i++) {
        state.playerBullets[i].Render(&program);
    }

    for (int i = 0; i < ASTEROID_COUNT; i++) {
        state.asteroids[i].Render(&program);
    }

    for (int i = 0; i < METEOR_COUNT; i++) {
        state.meteors[i].Render(&program);
    }

    for (int i = 0; i < UFO_MAX_BULLET; i++) {
        state.ufoBullets[i].Render(&program);
    }

    for (int i = 0; i < UFO_COUNT; i++) {
        state.ufos[i].Render(&program);
        if (state.ufos[i].active)
            Util::DrawText(&program, fontTextureID, "[" + to_string(state.ufos[i].hp) + "]", 0.4f, -0.25f, glm::vec3(state.ufos[i].position.x-0.1, state.ufos[i].position.y - 0.7, 0));
    }

    for (int i = 0; i < KAMIKAZE_COUNT; i++) {
        state.kamikazes[i].Render(&program);
        if (state.kamikazes[i].active)
            Util::DrawText(&program, fontTextureID, "[" + to_string(state.kamikazes[i].hp) + "]", 0.4f, -0.25f, glm::vec3(state.kamikazes[i].position.x - 0.1, state.kamikazes[i].position.y - 0.7, 0));
    }

    for (int i = 0; i < CLEVER_COUNT; i++) {
        state.clevers[i].Render(&program);
        if (state.clevers[i].active)
            Util::DrawText(&program, fontTextureID, "[" + to_string(state.clevers[i].hp) + "]", 0.4f, -0.25f, glm::vec3(state.clevers[i].position.x - 0.2, state.clevers[i].position.y - 0.4, 0));
    }


    for (int i = 0; i < COIN_COUNT; i++) {
        state.coins[i].Render(&program);
    }

    for (int i = 0; i < DIAMOND_COUNT; i++) {
        state.diamonds[i].Render(&program);
    }

    for (int i = 0; i < MAGNET_COUNT; i++) {
        state.magnets[i].Render(&program);
    }

    for (int i = 0; i < POWERUP_COUNT; i++) {
        state.powerups[i].Render(&program);
    }

    state.score->Render(&program);
    Util::DrawText(&program, fontTextureID, to_string(state.player->score), 0.6f, -0.30f, glm::vec3(3.25, state.player->position.y + 11.28, 0));
    if (state.player->magnet)
        Util::DrawText(&program, fontTextureID, "Activated!", 0.35f, -0.225f, glm::vec3(-4.25, state.player->position.y + 10.0, 0));

    Util::DrawText(&program, fontTextureID, "Tier " + to_string(upgrade), 0.35f, -0.225f, glm::vec3(3.25, state.player->position.y + 10.80, 0));
    switch (upgrade) {
    case 0:
        Util::DrawText(&program, fontTextureID, "Next Tier:50", 0.45f, -0.225f, glm::vec3(2.0, state.player->position.y - 1.75, 0));
        break;
    case 1:
        Util::DrawText(&program, fontTextureID, "Next Tier:200", 0.45f, -0.225f, glm::vec3(2.0, state.player->position.y - 1.75, 0));
        break;
    case 2:
        Util::DrawText(&program, fontTextureID, "Next Tier:500", 0.45f, -0.225f, glm::vec3(2.0, state.player->position.y - 1.75, 0));
        break;
    case 3:
        Util::DrawText(&program, fontTextureID, "Next Tier:1337", 0.45f, -0.225f, glm::vec3(2.0, state.player->position.y - 1.75, 0));
        break;
    case 4:
        Util::DrawText(&program, fontTextureID, "To Win:2000", 0.45f, -0.225f, glm::vec3(2.0, state.player->position.y - 1.75, 0));
        break;
    }
    state.armor->Render(&program);
    state.magnets[MAGNET_COUNT].Render(&program);
    Util::DrawText(&program, fontTextureID, "<" + to_string(upgrade) + ">", 0.4f, -0.25f, glm::vec3(state.player->position.x-0.175, state.player->position.y - 0.8, 0));
    state.player->Render(&program);

    // If lost all hp
    if (state.player->hp == 0) {
        Mix_HaltMusic();
        Mix_PlayChannel(-1, dead, 0);
        this_thread::sleep_for(chrono::milliseconds(1200));
        Mix_PlayChannel(-1, game_over, 0);
        mode = GameMode::GAME_OVER;
    }
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    program.SetViewMatrix(viewMatrix);

    switch (mode) {
    case GameMode::MAIN_MENU:
        RenderMainMenu();
        break;
    case GameMode::GAME_LEVEL:
        RenderGameLevel();
        break;
    case GameMode::GAME_OVER:
        Util::DrawText(&program, fontTextureID, "Game Over", 0.90f, -0.25f, glm::vec3(-2.60 , state.player->position.y+5, 0));
        break;
    case GameMode::GAME_WIN:
        RenderGameWin();
        break;
    }

    
    SDL_GL_SwapWindow(displayWindow);
}


void Shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();
    
    while (gameIsRunning) {
        ProcessInput();
        Update();
        Render();
    }
    
    Shutdown();
    return 0;
}
