#include "Entity.h"
#include "Util.h"
#include <string>

Entity::Entity()
{
    position;
    movement = glm::vec3(0);
    acceleration = glm::vec3(0);
    velocity = glm::vec3(0);
    speed = 0;
    rotate = 0;
    nextCoin = 0;
    score = 0;
    
    modelMatrix = glm::mat4(1.0f);
}

bool Entity::CheckCollision(Entity* other) {
    if (other->active == false) return false;
    float xdist = fabs(position.x - other->position.x) - ((width + other->width) / 2.0f);
    float ydist = fabs(position.y - other->position.y) - ((height + other->height) / 2.0f);

    if (xdist < 0 && ydist < 0) {
        if (other->entityType != EntityType::WALL) {
            if (entityType != EntityType::SHIP) {
                active = false;
            }
            collided = true;
        }
        return true;
    }

    return false;
}

void Entity::CheckCollisionsY(Entity* objects, int objectCount)
{
    for (int i = 0; i < objectCount; i++)
    {
        Entity* object = &objects[i];

        if (CheckCollision(object))
        {
            if (entityType == EntityType::SHIP) {
                hp -= 1;
                iframe = true;
                iframeStart = steady_clock::now();
                if (objects[i].aiType == AIType::KAMIKAZE) {
                    objects[i].active = false;
                    objects[i].collided = true;
                }
                return;
            }
            float ydist = fabs(position.y - object->position.y);
            float penetrationY = fabs(ydist - (height / 2.0f) - (object->height / 2.0f));
            if (velocity.y > 0) {
                position.y -= penetrationY;
                velocity.y = 0;
            }
            else if (velocity.y < 0) {
                position.y += penetrationY;
                velocity.y = 0;
            }
        }
    }
}

void Entity::CheckCollisionsX(Entity* objects, int objectCount)
{
    for (int i = 0; i < objectCount; i++)
    {
        Entity* object = &objects[i];

        if (CheckCollision(object))
        {
            float xdist = fabs(position.x - object->position.x);
            float penetrationX = fabs(xdist - (width / 2.0f) - (object->width / 2.0f));
            if (velocity.x > 0) {
                position.x -= penetrationX;
                velocity.x = 0;
            }
            else if (velocity.x < 0) {
                position.x += penetrationX;
                velocity.x = 0;
            }
            if (entityType == EntityType::ENEMY)
                movement *= -1;
        }
    }
}

void Entity::AIUfo(Entity* player, Entity* ufoBullets, int ufoBulletCount) {
    if (position.y - player->position.y < 10) {
        velocity.y = 2.0f;
        speed = 1.0f;
        auto current = steady_clock::now();
        if (duration_cast<seconds>(current - ufoBulletStart).count() > 1.0) {
            ufoBulletStart = steady_clock::now();
            ufoBullets[nextUfoBullet].position = glm::vec3(position.x, position.y - 0.2, 0);
            ufoBullets[nextUfoBullet].velocity.y = -5.0f;
            ufoBullets[nextUfoBullet].rotate = 0;
            ufoBullets[nextUfoBullet].active = true;
            nextUfoBullet++;
            ufoBullets[nextUfoBullet].position = glm::vec3(position.x + 0.2, position.y - 0.2, 0);
            ufoBullets[nextUfoBullet].velocity.y = -4.75f;
            ufoBullets[nextUfoBullet].movement.x = 1.56f;
            ufoBullets[nextUfoBullet].rotate = 0.26;
            ufoBullets[nextUfoBullet].active = true;
            nextUfoBullet++;
            ufoBullets[nextUfoBullet].position = glm::vec3(position.x - 0.2, position.y - 0.2, 0);
            ufoBullets[nextUfoBullet].velocity.y = -4.75f;
            ufoBullets[nextUfoBullet].movement.x = -1.56f;
            ufoBullets[nextUfoBullet].rotate = -0.26;
            ufoBullets[nextUfoBullet].active = true;
            nextUfoBullet++;
            if (nextUfoBullet == ufoBulletCount) nextUfoBullet = 0;
        }
    }
    else
        velocity.y = -2.0f;

}

void Entity::AIKamikaze(Entity* player) {
    if (position.y - player->position.y < 11) {
        int x_disp = position.x - player->position.x;
        int y_disp = position.y - player->position.y;
        if (pow(y_disp, 2) + pow(x_disp, 2) < 3.5) {
            velocity.y = y_disp * -30;
            speed = x_disp * -30;
        }
        else {
            velocity.y = y_disp * -1.0;
            speed = x_disp * -1.0;
        }
    }
}

void Entity::AIClever(Entity* player) {
    if (position.y - player->position.y < 8) {
        if (!goingDown) {
            velocity.y = 2.0f;
            speed = 1.0f;
        }
        auto current = steady_clock::now();
        if (duration_cast<seconds>(current - cleverAttackStart).count() > 8.0) {
            cleverAttackStart = steady_clock::now();
            goingDown = true;
            velocity.y = -6.0f;
            speed = 0.0f;
        }
        if (position.y < player->position.y) {
            velocity.y = 6.0f;
        }
    }
    else {
        velocity.y = -2.0f;
        goingDown = false;
    }
}

void Entity::AI(Entity* player, Entity* walls, int wallCount, Entity* ufoBullets, int ufoBulletCount) {
    CheckCollisionsX(walls, wallCount);

    switch (aiType) {
    case AIType::UFO:
        AIUfo(player, ufoBullets, ufoBulletCount);
        break;
    case AIType::KAMIKAZE:
        AIKamikaze(player);
        break;
    
    case AIType::CLEVER:
        AIClever(player);
        break;
    }
}

void Entity::Update(float deltaTime, Entity* player, Entity* asteroids, int asteroidCount, Entity* meteors, int meteorCount, Entity* walls, int wallCount, Entity* ufoBullets, int ufoBulletCount, Entity* ufos, int ufoCount, Entity* coins, int coinCount, Entity* diamonds, int diamondCount, Entity* magnets, int magnetCount, Entity* powerups, int powerupCount, Entity* kamikazes, int kamikazeCount, Entity* clevers, int cleverCount)
{
    if (active == false) return;

    if (entityType == EntityType::ARMOR && player->hp >= 0) {
        animIndex = 4 - player->hp;
    }

    if (entityType == EntityType::ENEMY) {
        AI(player, walls, wallCount, ufoBullets, ufoBulletCount);
    }

    if (entityType == EntityType::PLAYERBULLET) {
        for (int i = 0; i < asteroidCount; i++) {
            if (CheckCollision(&asteroids[i])) {
                collided = true;
                asteroids[i].hp -= 1;
                if (asteroids[i].hp == 0) {
                    asteroids[i].active = false;
                    random_device rd;
                    default_random_engine generator(rd());
                    uniform_int_distribution<int> distribution(1, 5);
                    int num = distribution(generator);
                    if (num == 1) {
                        magnets[player->nextMagnet].acceleration = glm::vec3(0, -3.0f, 0);
                        magnets[player->nextMagnet].coinDropSound = true;
                        magnets[player->nextMagnet].velocity.y = 3.0f;
                        magnets[player->nextMagnet].movement.x = num * 0.05 * distribution(generator) * 0.5;
                        magnets[player->nextMagnet].active = true;
                        magnets[player->nextMagnet].collided = true;
                        magnets[player->nextMagnet].position = glm::vec3(asteroids[i].position.x + (0.05 * 1 * 2), asteroids[i].position.y + (0.05 * 1), 0);
                        player->nextMagnet++;
                        if (player->nextMagnet == magnetCount) player->nextMagnet = 0;
                    }
                    for (int j = 0; j < num; j++) {
                        random_device rd;
                        default_random_engine generator(rd());
                        uniform_real_distribution<double> distribution(-0.15, 0.15);
                        double pos = distribution(generator);
                        coins[player->nextCoin].acceleration = glm::vec3(0, -3.0f, 0);
                        coins[player->nextCoin].coinDropSound = true;
                        coins[player->nextCoin].velocity.y = 3.0f;
                        coins[player->nextCoin].movement.x = num * j * distribution(generator) * 0.5;
                        coins[player->nextCoin].active = true;
                        coins[player->nextCoin].collided = true;
                        coins[player->nextCoin].position = glm::vec3(asteroids[i].position.x + (pos*j*2), asteroids[i].position.y + (pos*j), 0);
                        player->nextCoin++;
                        if (player->nextCoin == coinCount) player->nextCoin = 0;
                    }
                }
            }
        }
        for (int i = 0; i < ufoCount; i++) {
            if (ufos[i].position.y - player->position.y < 10) {
                if (CheckCollision(&ufos[i])) {
                    collided = true;
                    ufos[i].hp -= 1;
                    if (ufos[i].hp == 0) {
                        ufos[i]. collided = true;
                        ufos[i].active = false;
                        random_device rd;
                        default_random_engine generator(rd());
                        uniform_int_distribution<int> distribution(3, 8);
                        int num = distribution(generator);
                        if (num == 3 || num == 4 || num == 5 || num == 6) {
                            magnets[player->nextMagnet].acceleration = glm::vec3(0, -3.0f, 0);
                            magnets[player->nextMagnet].coinDropSound = true;
                            magnets[player->nextMagnet].velocity.y = 3.0f;
                            magnets[player->nextMagnet].movement.x = num * 0.05 * distribution(generator) * 0.5;
                            magnets[player->nextMagnet].active = true;
                            magnets[player->nextMagnet].collided = true;
                            magnets[player->nextMagnet].position = glm::vec3(ufos[i].position.x + (0.05 * 1 * 2), ufos[i].position.y + (0.05 * 1), 0);
                            player->nextMagnet++;
                            if (player->nextMagnet == magnetCount) player->nextMagnet = 0;
                        }
                        if (num == 3 || num == 5 ) {
                            powerups[player->nextPowerup].acceleration = glm::vec3(0, -3.0f, 0);
                            powerups[player->nextPowerup].coinDropSound = true;
                            powerups[player->nextPowerup].velocity.y = 3.0f;
                            powerups[player->nextPowerup].movement.x = num * 0.05 * distribution(generator) * 0.5;
                            powerups[player->nextPowerup].active = true;
                            powerups[player->nextPowerup].collided = true;
                            powerups[player->nextPowerup].position = glm::vec3(ufos[i].position.x + (0.05 * 1 * 2), ufos[i].position.y + (0.05 * 1), 0);
                            player->nextPowerup++;
                            if (player->nextPowerup == powerupCount) player->nextPowerup = 0;
                        }
                        for (int j = 0; j < num; j++) {
                            random_device rd;
                            default_random_engine generator(rd());
                            uniform_real_distribution<double> distribution(-0.15, 0.15);
                            double pos = distribution(generator);
                            coins[player->nextCoin].acceleration = glm::vec3(0, -3.0f, 0);
                            coins[player->nextCoin].coinDropSound = true;
                            coins[player->nextCoin].velocity.y = 3.0f;
                            coins[player->nextCoin].movement.x = num * j * distribution(generator) * 0.5;
                            coins[player->nextCoin].active = true;
                            coins[player->nextCoin].collided = true;
                            coins[player->nextCoin].position = glm::vec3(ufos[i].position.x + (pos * j * 2), ufos[i].position.y + (pos * j), 0);
                            player->nextCoin++;
                            if (player->nextCoin == coinCount) player->nextCoin = 0;
                        }
                        uniform_int_distribution<int> distributions(1, 3);
                        int num2 = distributions(generator);
                        for (int j = 0; j < num2; j++) {
                            random_device rd;
                            default_random_engine generator(rd());
                            uniform_real_distribution<double> distribution(-0.15, 0.15);
                            double pos = distribution(generator);
                            diamonds[player->nextDiamond].acceleration = glm::vec3(0, -3.0f, 0);
                            diamonds[player->nextDiamond].coinDropSound = true;
                            diamonds[player->nextDiamond].velocity.y = 3.0f;
                            diamonds[player->nextDiamond].movement.x = num2 * j * distribution(generator) * 0.5;
                            diamonds[player->nextDiamond].active = true;
                            diamonds[player->nextDiamond].collided = true;
                            diamonds[player->nextDiamond].position = glm::vec3(ufos[i].position.x + (pos * j * 2), ufos[i].position.y + (pos * j), 0);
                            player->nextDiamond++;
                            if (player->nextDiamond == diamondCount) player->nextDiamond = 0;
                        }
                    }
                }
            }
        }
        for (int i = 0; i < kamikazeCount; i++) {
            if (CheckCollision(&kamikazes[i])) {
                collided = true;
                kamikazes[i].hp -= 1;
                if (kamikazes[i].hp == 0) {
                    kamikazes[i].collided = true;
                    kamikazes[i].active = false;
                    random_device rd;
                    default_random_engine generator(rd());
                    uniform_int_distribution<int> distribution(2, 4);
                    int num = distribution(generator);
                    if (num == 3 || num == 4) {
                        magnets[player->nextMagnet].acceleration = glm::vec3(0, -3.0f, 0);
                        magnets[player->nextMagnet].coinDropSound = true;
                        magnets[player->nextMagnet].velocity.y = 3.0f;
                        magnets[player->nextMagnet].movement.x = num * 0.05 * distribution(generator) * 0.5;
                        magnets[player->nextMagnet].active = true;
                        magnets[player->nextMagnet].collided = true;
                        magnets[player->nextMagnet].position = glm::vec3(kamikazes[i].position.x + (0.05 * 1 * 2), kamikazes[i].position.y + (0.05 * 1), 0);
                        player->nextMagnet++;
                        if (player->nextMagnet == magnetCount) player->nextMagnet = 0;
                    }
                    if (num == 2) {
                        powerups[player->nextPowerup].acceleration = glm::vec3(0, -3.0f, 0);
                        powerups[player->nextPowerup].coinDropSound = true;
                        powerups[player->nextPowerup].velocity.y = 3.0f;
                        powerups[player->nextPowerup].movement.x = num * 0.05 * distribution(generator) * 0.5;
                        powerups[player->nextPowerup].active = true;
                        powerups[player->nextPowerup].collided = true;
                        powerups[player->nextPowerup].position = glm::vec3(kamikazes[i].position.x + (0.05 * 1 * 2), kamikazes[i].position.y + (0.05 * 1), 0);
                        player->nextPowerup++;
                        if (player->nextPowerup == powerupCount) player->nextPowerup = 0;
                    }
                    for (int j = 0; j < num; j++) {
                        random_device rd;
                        default_random_engine generator(rd());
                        uniform_real_distribution<double> distribution(-0.15, 0.15);
                        double pos = distribution(generator);
                        coins[player->nextCoin].acceleration = glm::vec3(0, -3.0f, 0);
                        coins[player->nextCoin].coinDropSound = true;
                        coins[player->nextCoin].velocity.y = 3.0f;
                        coins[player->nextCoin].movement.x = num * j * distribution(generator) * 0.5;
                        coins[player->nextCoin].active = true;
                        coins[player->nextCoin].collided = true;
                        coins[player->nextCoin].position = glm::vec3(kamikazes[i].position.x + (pos * j * 2), kamikazes[i].position.y + (pos * j), 0);
                        player->nextCoin++;
                        if (player->nextCoin == coinCount) player->nextCoin = 0;
                    }
                    uniform_int_distribution<int> distributions(0, 1);
                    int num2 = distributions(generator);
                    for (int j = 0; j < num2; j++) {
                        random_device rd;
                        default_random_engine generator(rd());
                        uniform_real_distribution<double> distribution(-0.15, 0.15);
                        double pos = distribution(generator);
                        diamonds[player->nextDiamond].acceleration = glm::vec3(0, -3.0f, 0);
                        diamonds[player->nextDiamond].coinDropSound = true;
                        diamonds[player->nextDiamond].velocity.y = 3.0f;
                        diamonds[player->nextDiamond].movement.x = num2 * j * distribution(generator) * 0.5;
                        diamonds[player->nextDiamond].active = true;
                        diamonds[player->nextDiamond].collided = true;
                        diamonds[player->nextDiamond].position = glm::vec3(kamikazes[i].position.x + (pos * j * 2), kamikazes[i].position.y + (pos * j), 0);
                        player->nextDiamond++;
                        if (player->nextDiamond == diamondCount) player->nextDiamond = 0;
                    }
                }
            }
        }

        for (int i = 0; i < cleverCount; i++) {
            if (CheckCollision(&clevers[i])) {
                collided = true;
                clevers[i].hp -= 1;
                if (clevers[i].hp == 0) {
                    clevers[i].collided = true;
                    clevers[i].active = false;
                    random_device rd;
                    default_random_engine generator(rd());
                    uniform_int_distribution<int> distribution(6, 10);
                    int num = distribution(generator);
                    if (num == 6 || num == 7 || num == 8 || num == 9) {
                        magnets[player->nextMagnet].acceleration = glm::vec3(0, -3.0f, 0);
                        magnets[player->nextMagnet].coinDropSound = true;
                        magnets[player->nextMagnet].velocity.y = 3.0f;
                        magnets[player->nextMagnet].movement.x = num * 0.05 * distribution(generator) * 0.5;
                        magnets[player->nextMagnet].active = true;
                        magnets[player->nextMagnet].collided = true;
                        magnets[player->nextMagnet].position = glm::vec3(clevers[i].position.x + (0.05 * 1 * 2), clevers[i].position.y + (0.05 * 1), 0);
                        player->nextMagnet++;
                        if (player->nextMagnet == magnetCount) player->nextMagnet = 0;
                    }
                    powerups[player->nextPowerup].acceleration = glm::vec3(0, -3.0f, 0);
                    powerups[player->nextPowerup].coinDropSound = true;
                    powerups[player->nextPowerup].velocity.y = 3.0f;
                    powerups[player->nextPowerup].movement.x = num * 0.05 * distribution(generator) * 0.5;
                    powerups[player->nextPowerup].active = true;
                    powerups[player->nextPowerup].collided = true;
                    powerups[player->nextPowerup].position = glm::vec3(clevers[i].position.x + (0.05 * 1 * 2), clevers[i].position.y + (0.05 * 1), 0);
                    player->nextPowerup++;
                    if (player->nextPowerup == powerupCount) player->nextPowerup = 0;
                    for (int j = 0; j < num; j++) {
                        random_device rd;
                        default_random_engine generator(rd());
                        uniform_real_distribution<double> distribution(-0.15, 0.15);
                        double pos = distribution(generator);
                        coins[player->nextCoin].acceleration = glm::vec3(0, -3.0f, 0);
                        coins[player->nextCoin].coinDropSound = true;
                        coins[player->nextCoin].velocity.y = 3.0f;
                        coins[player->nextCoin].movement.x = num * j * distribution(generator) * 0.5;
                        coins[player->nextCoin].active = true;
                        coins[player->nextCoin].collided = true;
                        coins[player->nextCoin].position = glm::vec3(clevers[i].position.x + (pos * j * 2), clevers[i].position.y + (pos * j), 0);
                        player->nextCoin++;
                        if (player->nextCoin == coinCount) player->nextCoin = 0;
                    }
                    uniform_int_distribution<int> distributions(3, 5);
                    int num2 = distributions(generator);
                    for (int j = 0; j < num2; j++) {
                        random_device rd;
                        default_random_engine generator(rd());
                        uniform_real_distribution<double> distribution(-0.15, 0.15);
                        double pos = distribution(generator);
                        diamonds[player->nextDiamond].acceleration = glm::vec3(0, -3.0f, 0);
                        diamonds[player->nextDiamond].coinDropSound = true;
                        diamonds[player->nextDiamond].velocity.y = 3.0f;
                        diamonds[player->nextDiamond].movement.x = num2 * j * distribution(generator) * 0.5;
                        diamonds[player->nextDiamond].active = true;
                        diamonds[player->nextDiamond].collided = true;
                        diamonds[player->nextDiamond].position = glm::vec3(clevers[i].position.x + (pos * j * 2), clevers[i].position.y + (pos * j), 0);
                        player->nextDiamond++;
                        if (player->nextDiamond == diamondCount) player->nextDiamond = 0;
                    }
                }
            }
        }
    }

    if (entityType == EntityType::COIN && coinType != CoinType::ICON) {
        int x_disp = position.x - player->position.x;
        int y_disp = position.y - player->position.y;
        if ((pow(y_disp, 2) + pow(x_disp, 2) < 3.3) && player->magnet) {
            velocity.y = y_disp * -50;
            movement.x = x_disp * -50;
            acceleration = glm::vec3(0);
        }
        if (CheckCollision(player)) {
            collided = true;
            active = false;
            coinSound = true;
            if (coinType == CoinType::DIAMOND)
                player->score += 5;
            else if (coinType == CoinType::COIN)
                player->score++;
            else if (coinType == CoinType::MAGNET) {
                if (player->magnet)
                    player->magnetTime += 10.0f;
                player->magnet = true;
                player->magnetStart = steady_clock::now();
            }
            else {
                if (player->hp != 4)
                    player->hp++;
            }
        }
    }

    if (spriteIndices != NULL) {
        if (glm::length(velocity) != 0) {
            spriteTime += deltaTime;

            if (spriteTime >= 0.25f)
            {
                spriteTime = 0.0f;
                spriteIndex++;
                std::string text = "Meteor" + std::to_string(spriteIndex) + ".png";
                textureID = Util::LoadTexture(text.c_str());
                if (spriteIndex >= 3)
                {
                    spriteIndex = 1;
                    std::string text = "Meteor" + std::to_string(spriteIndex) + ".png";
                    textureID = Util::LoadTexture(text.c_str());
                }
            }
        } else {
            spriteIndex = 0;
        }
    }
   
    
    velocity.x = movement.x * speed;
    velocity += acceleration * deltaTime;

    position.y += velocity.y * deltaTime; // Move on Y
    if (entityType == EntityType::SHIP && !iframe) {
        CheckCollisionsY(asteroids, asteroidCount);
        CheckCollisionsY(meteors, meteorCount);
        CheckCollisionsY(ufoBullets, ufoBulletCount);
        CheckCollisionsY(kamikazes, kamikazeCount);
        CheckCollisionsY(clevers, cleverCount);
    }

    position.x += velocity.x * deltaTime; // Move on X
    CheckCollisionsX(walls, wallCount);

    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, rotate, glm::vec3(0, 0, 1));
}

void Entity::DrawSpriteFromTextureAtlas(ShaderProgram *program, GLuint textureID, int index)
{
    float u = (float)(index % animCols) / (float)animCols;
    float v = (float)(index / animCols) / (float)animRows;
    
    float width = 1.0f / (float)animCols;
    float height = 1.0f / (float)animRows;
    
    float texCoords[] = { u, v + height, u + width, v + height, u + width, v,
        u, v + height, u + width, v, u, v};
    
    if (entityType == EntityType::ARMOR) {
        float vertices[] = { -1.6, -0.40, 1.6, -0.40, 1.6, 0.40, -1.6, -0.40, 1.6, 0.40, -1.6, 0.40 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }
    else if (entityType == EntityType::SCORE) {
        float vertices[] = { -1.5, -0.40, 1.5, -0.4, 1.5, 0.40, -1.5, -0.40, 1.5, 0.40, -1.5, 0.40 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }
    else {
        float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void Entity::Render(ShaderProgram* program) {
    if (active == false) return;

    program->SetModelMatrix(modelMatrix);

    if (animIndices != NULL) {
        DrawSpriteFromTextureAtlas(program, textureID, animIndices[animIndex]);
        return;
    }

    if (entityType == EntityType::SHIP) {
        float vertices[] = { -0.65, -0.65, 0.65, -0.65, 0.65, 0.65, -0.65, -0.65, 0.65, 0.65, -0.65, 0.65 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }

    else if (entityType == EntityType::PLAYERBULLET) {
        float vertices[] = { -0.15, -0.15, 0.15, -0.15, 0.15, 0.15, -0.15, -0.15, 0.15, 0.15, -0.15, 0.15 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }

    else if (entityType == EntityType::ASTEROID) {
        float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }

    else if (entityType == EntityType::METEOR) {
        float vertices[] = { -0.50, -0.50, 0.50, -0.50, 0.50, 0.50, -0.50, -0.50, 0.50, 0.50, -0.50, 0.50 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }

    else if (entityType == EntityType::BACKGROUND) {
        float vertices[] = { -5.0, -7.0, 5.0, -7.0, 5.0, 7.0, -5.0, -7.0, 5.0, 7.0, -5.0, 7.0 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }

    else if (aiType == AIType::UFO) {
        float vertices[] = { -0.8, -0.5, 0.8, -0.5, 0.8, 0.5, -0.8, -0.5, 0.8, 0.5, -0.8, 0.5 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }

    else if (aiType == AIType::KAMIKAZE) {
        float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }

    else if (aiType == AIType::CLEVER) {
        float vertices[] = { -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0, 1.0 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }

    else if (entityType == EntityType::UFOBULLET) {
        float vertices[] = { -0.1, -0.15, 0.1, -0.15, 0.1, 0.15, -0.1, -0.15, 0.1, 0.15, -0.1, 0.15 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }

    else if (entityType == EntityType::COIN && coinType != CoinType::ICON) {
        float vertices[] = { -0.25, -0.25, 0.25, -0.25, 0.25, 0.25, -0.25, -0.25, 0.25, 0.25, -0.25, 0.25 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }

    else if (entityType == EntityType::COIN && coinType == CoinType::ICON) {
        float vertices[] = { -0.35, -0.35, 0.35, -0.35, 0.35, 0.35, -0.35, -0.35, 0.35, 0.35, -0.35, 0.35 };
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    }


    float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
    glBindTexture(GL_TEXTURE_2D, textureID);
    

    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}
