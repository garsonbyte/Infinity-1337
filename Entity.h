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
#include <chrono>
#include <random>
#include <math.h>

using namespace std;
using namespace chrono;

enum class EntityType {SHIP, PLAYERBULLET, ASTEROID, METEOR, BACKGROUND, ENEMY, WALL, UFOBULLET, COIN, ARMOR, SCORE};
enum class CoinType { COIN, DIAMOND, MAGNET, ICON, POWERUP };
enum class AIType { UFO, KAMIKAZE, CLEVER };

class Entity {
public:
    EntityType entityType;
    AIType aiType;
    CoinType coinType;
    glm::vec3 position;
    glm::vec3 movement;
    glm::vec3 acceleration;
    glm::vec3 velocity;

    steady_clock::time_point ufoBulletStart;
    steady_clock::time_point cleverAttackStart;
    bool goingDown = false;
    steady_clock::time_point iframeStart;
    steady_clock::time_point magnetStart;
    int nextUfoBullet;

    //Hitpoints
    int hp;

    // Score
    int score;

    //Coins
    int nextCoin;
    bool coinDropSound = false;
    bool coinSound = false;
    int nextDiamond;
    int nextMagnet;
    int nextPowerup;
    int magnetTime = 10.0f;
    bool magnet = false;

    float width = 1.0f;
    float height = 1.0f;

    float speed;
    float rotate;
    
    bool active = true;
    bool collided = false;
    bool restart = false;
    bool iframe;
    
    GLuint textureID;
    
    glm::mat4 modelMatrix;
    
    int *animRight = NULL;
    int *animLeft = NULL;
    int *animUp = NULL;
    int *animDown = NULL;

    int *animIndices = NULL;
    int animFrames = 0;
    int animIndex = 0;
    float animTime = 0;
    int animCols = 0;
    int animRows = 0;

    // Sprite index to keep track of sprite png using
    int* spriteIndices = NULL;
    int spriteIndex = 1;
    float spriteTime = 0;
    
    Entity();
    bool CheckCollision(Entity* other);
    void CheckCollisionsY(Entity* objects, int objectCount);
    void CheckCollisionsX(Entity* objects, int objectCount);
    void AIUfo(Entity* player, Entity* ufoBullets, int ufoBulletCount);
    void AIKamikaze(Entity* player);
    void AIClever(Entity* player);
    void AI(Entity* player, Entity* walls, int wallCount, Entity* ufoBullets, int ufoBulletCount);
    void Update(float deltaTime, Entity* player, Entity* asteroids, int asteroidCount, Entity* meteors, int meteorCount, Entity* walls, int wallCount, Entity* ufoBullets, int ufoBulletCount, Entity* ufos, int ufoCount, Entity* coins, int coinCount, Entity* diamonds, int diamondCount, Entity* magnets, int magnetCount, Entity* powerups, int powerupCount, Entity* kamikazes, int kamikazeCount, Entity* clevers, int cleverCount);
    void Render(ShaderProgram *program);
    void DrawSpriteFromTextureAtlas(ShaderProgram *program, GLuint textureID, int index);
};
