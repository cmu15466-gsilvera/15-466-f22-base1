#include "PlayMode.hpp"

// for the GL_ERRORS() macro:
#include "gl_errors.hpp"

// for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

// load_save_png
#include "load_save_png.hpp"

#include <algorithm> // std::clamp
#include <random>

PlayMode::PlayMode()
{

    int globalSpriteIndex = 0; // increases with every new sprite

    // meta stuff
    {
        // attribute 0 is undrawn
        ppu.palette_table[CLEAR_COLOUR] = {
            glm::u8vec4(0, 0, 0, 0),
            glm::u8vec4(0, 0, 0, 0),
            glm::u8vec4(0, 0, 0, 0),
            glm::u8vec4(0, 0, 0, 0),
        };
    }

    // main character
    {
        // load png
        glm::uvec2 size;
        std::vector<glm::u8vec4> data;
        load_png("assets/siphon.png", &size, &data);
        std::vector<glm::u8vec4> colour_bank = {
            glm::u8vec4(0, 0, 0, 0),
            glm::u8vec4(128, 128, 128, 255),
            glm::u8vec4(255, 0, 0, 255),
            glm::u8vec4(0, 0, 0, 0),
        };
        // convert_to_n_colours(4, size, &(data[0]), colour_bank);
        convert_to_new_size_with_bank(glm::uvec2(8, 8), size, data, colour_bank);

        siphon_sd = SpriteData(data, colour_bank, true);

        // initialize siphon (player) data
        siphon.spriteID = globalSpriteIndex;
        siphon.speed = 80.f;
        globalSpriteIndex++;
        siphon.pos.x = PPU466::ScreenWidth / 2;
        siphon.pos.y = PPU466::ScreenHeight / 2;
        siphon.sprite.index = SIPHON_SPRITE_IDX;
        siphon.sprite.attributes = SIPHON_COLOUR;
        ppu.tile_table[siphon.sprite.index] = siphon_sd.GetBits();
        ppu.palette_table[siphon.sprite.attributes] = siphon_sd.colours;
    }

    // projectiles
    {
        projectiles.reserve(numProjectiles);
        // load png
        glm::uvec2 size;
        std::vector<glm::u8vec4> data;
        load_png("assets/bolt.png", &size, &data);
        std::vector<glm::u8vec4> colour_bank = {
            glm::u8vec4(0, 0, 0, 0),
            glm::u8vec4(255, 255, 0, 255),
            glm::u8vec4(0, 0, 0, 0),
            glm::u8vec4(0, 0, 0, 0),
        };
        convert_to_new_size_with_bank(glm::uvec2(8, 8), size, data, colour_bank);

        SpriteData projectile_sd = SpriteData(data, colour_bank, true);
        // colours used for the misc other sprites:
        ppu.palette_table[PROJECTILE_COLOUR] = projectile_sd.colours;
        ppu.tile_table[PROJECTILE_SPRITE_IDX_0] = projectile_sd.GetBits(0);
        ppu.tile_table[PROJECTILE_SPRITE_IDX_1] = projectile_sd.GetBits(1); // rotated 90 deg (horizontal)

        for (size_t i = 0; i < numProjectiles; i++) {
            MovingObject newProj;
            newProj.speed = 50.f;
            newProj.spriteID = globalSpriteIndex;
            globalSpriteIndex++;
            newProj.sprite.index = PROJECTILE_SPRITE_IDX_0;
            newProj.sprite.attributes = PROJECTILE_COLOUR;
            newProj.randomInit();
            projectiles.push_back(newProj);
        }
    }

    // targets
    {
        targets.reserve(numTargets);
        // load png
        glm::uvec2 size;
        std::vector<glm::u8vec4> data;
        load_png("assets/target.png", &size, &data);
        std::vector<glm::u8vec4> colour_bank = {
            glm::u8vec4(0, 0, 0, 0),
            glm::u8vec4(255, 0, 0, 255),
            glm::u8vec4(255, 255, 255, 255),
            glm::u8vec4(0, 0, 0, 0),
        };
        convert_to_new_size_with_bank(glm::uvec2(8, 8), size, data, colour_bank);

        SpriteData target_sd = SpriteData(data, colour_bank, false);
        // colours used for the misc other sprites:
        ppu.palette_table[TARGET_COLOUR] = target_sd.colours;
        ppu.tile_table[TARGET_SPRITE_IDX] = target_sd.GetBits();

        for (size_t i = 0; i < numTargets; i++) {
            MovingObject newTarget;
            newTarget.speed = 30.f;
            newTarget.spriteID = globalSpriteIndex;
            globalSpriteIndex++;
            newTarget.sprite.index = TARGET_SPRITE_IDX;
            newTarget.sprite.attributes = TARGET_COLOUR;
            newTarget.randomInit();
            targets.push_back(newTarget);
        }
    }

    // super targets
    {
        superTargets.reserve(numSuperTargets);
        // load png
        glm::uvec2 size;
        std::vector<glm::u8vec4> data;
        load_png("assets/target.png", &size, &data);
        std::vector<glm::u8vec4> colour_bank = {
            glm::u8vec4(0, 0, 0, 0),
            glm::u8vec4(255, 0, 255, 255),
            glm::u8vec4(255, 255, 255, 255),
            glm::u8vec4(0, 0, 0, 0),
        };
        convert_to_new_size_with_bank(glm::uvec2(8, 8), size, data, colour_bank);

        SpriteData target_sd = SpriteData(data, colour_bank, false);
        // colours used for the misc other sprites:
        ppu.palette_table[SUPER_TARGET_COLOUR] = target_sd.colours;
        ppu.tile_table[TARGET_SPRITE_IDX] = target_sd.GetBits();

        for (size_t i = 0; i < numSuperTargets; i++) {
            MovingObject newTarget;
            newTarget.speed = 20.f;
            newTarget.spriteID = globalSpriteIndex;
            globalSpriteIndex++;
            newTarget.sprite.index = TARGET_SPRITE_IDX;
            newTarget.sprite.attributes = SUPER_TARGET_COLOUR;
            newTarget.randomInit();
            newTarget.hide(3 + rand() % 3);
            superTargets.push_back(newTarget);
        }
    }

    // background
    {
        ppu.palette_table[BACKGROUND_COLOUR] = {
            glm::u8vec4(0x11, 0x11, 0x11, 0xff),
            glm::u8vec4(0x11, 0x11, 0x11, 0xff),
            glm::u8vec4(0x11, 0x11, 0x99, 0xff),
            glm::u8vec4(0xff, 0xff, 0x11, 0xff),
        };

        for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
            for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
                int bkg_idx = SIPHON_SPRITE_IDX;
                ppu.background[x + PPU466::BackgroundWidth * y] = (BACKGROUND_COLOUR << 8) | bkg_idx;
            }
        }
    }
}

PlayMode::~PlayMode()
{
}

bool PlayMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size)
{
    bool wasSuccess = false;
    if (evt.type == SDL_KEYDOWN) {
        for (auto& key_action : key_assignment) {
            if (evt.key.keysym.sym == key_action.second) {
                key_action.first.pressed = true;
                wasSuccess = true;
            }
        }
    } else if (evt.type == SDL_KEYUP) {
        for (auto& key_action : key_assignment) {
            if (evt.key.keysym.sym == key_action.second) {
                key_action.first.pressed = false;
                wasSuccess = true;
            }
        }
    }

    return wasSuccess;
}

void PlayMode::PlayerUpdate(float dt)
{
    if (left.pressed) {
        siphon.vel.x = -siphon.speed;
    } else if (right.pressed) {
        siphon.vel.x = +siphon.speed;
    } else {
        siphon.vel.x = 0;
    }
    if (down.pressed) {
        siphon.vel.y = -siphon.speed;
    } else if (up.pressed) {
        siphon.vel.y = +siphon.speed;
    } else {
        siphon.vel.y = 0;
    }
    if (aim_left.pressed) {
        siphon.aimDirection = 2;
    }
    if (aim_right.pressed) {
        siphon.aimDirection = 0;
    }
    if (aim_down.pressed) {
        siphon.aimDirection = 1;
    }
    if (aim_up.pressed) {
        siphon.aimDirection = 3;
    }
    ppu.tile_table[siphon.sprite.index] = siphon_sd.GetBits(siphon.aimDirection);

    siphon.pos += dt * siphon.vel;

    siphon.pos.x = std::max(1.f, std::min(float(PPU466::ScreenWidth - 8), siphon.pos.x));
    siphon.pos.y = std::max(1.f, std::min(float(PPU466::ScreenHeight - 8), siphon.pos.y));
}

void PlayMode::ProjectileUpdate(float dt)
{
    for (MovingObject& p : projectiles) {
        // change the sprite based on velocity (heading direction)
        if (p.vel.y != 0) {
            p.sprite.index = PROJECTILE_SPRITE_IDX_0;
        } else {
            p.sprite.index = PROJECTILE_SPRITE_IDX_1;
        }
        p.update(dt);
        // check for collisions with player
        if (siphon.collisionWith(p)) {
            if (!p.collision) {
                // only trigger this effect on the FIRST frame of collision
                p.pos = siphon.pos;
                p.vel = p.speed * p.directionMapping(siphon.aimDirection);
            }
            p.collision = true;
        } else {
            p.collision = false;
        }
    }
}

void PlayMode::TargetsUpdate(float dt)
{
    for (MovingObject& t : targets) {
        t.update(dt);
        for (MovingObject& p : projectiles) {
            if (p.collisionWith(t)) {
                t.hide(5); // hide this target for the next 5s
                p.hide(2); // hide this projectile for the next 2s
                score++;
                std::cout << "[GOOD] Score: " << score << " ... Remaining: " << time_left << "s"<< std::endl;
            }
        }
    }

    for (MovingObject& t : superTargets) {
        // make the edge respawn wait for a bit before respawning
        t.update(dt);
        for (MovingObject& p : projectiles) {
            if (p.collisionWith(t)) {
                t.hide(5);  // hide this target for the next 5s
                p.hide(2);  // hide this projectile for the next 2s
                score += 5; // super points
                std::cout << "[SUPER] Score: " << score << " ... Remaining: " << time_left << "s" << std::endl;
            }
        }
    }
}

void PlayMode::update(float dt)
{

    // slowly rotates through [0,1):
    //  (will be used to set background color)
    background_fade += dt / 10.0f;
    background_fade -= std::floor(background_fade);

    // tick down the game-over timer
    time_left -= dt;

    if (time_left > 0){
        PlayerUpdate(dt);

        ProjectileUpdate(dt);

        TargetsUpdate(dt);
    } else {
        if (!end_msg){
            std::cout << "Game over! Final score: " << score << std::endl;
            end_msg = true;
        }
    }
}

void PlayMode::draw(glm::uvec2 const& drawable_size)
{
    //--- set ppu state based on game state ---

    // background color will be some hsv-like fade:
    ppu.background_color = glm::u8vec4(
        std::min(255, std::max(0, int32_t(255 * 0.5f * (0.5f + std::sin(2.0f * M_PI * (background_fade + 0.0f / 3.0f)))))),
        std::min(255, std::max(0, int32_t(255 * 0.5f * (0.5f + std::sin(2.0f * M_PI * (background_fade + 1.0f / 3.0f)))))),
        std::min(255, std::max(0, int32_t(255 * 0.5f * (0.5f + std::sin(2.0f * M_PI * (background_fade + 2.0f / 3.0f)))))),
        0xff);

    // background scroll:
    ppu.background_position += MovingObject::directionMapping(siphon.aimDirection);

    // player sprite:
    siphon.sprite.x = siphon.pos.x;
    siphon.sprite.y = siphon.pos.y;
    ppu.sprites[siphon.spriteID] = siphon.sprite;

    // projectile sprites
    for (const MovingObject& p : projectiles) {
        p.updatePPU(ppu);
        // if (i % 2)
        //     ppu.sprites[i].attributes |= 0x80; //'behind' bit
    }

    for (const MovingObject& t : targets) {
        t.updatePPU(ppu);
        // if (i % 2)
        //     ppu.sprites[i].attributes |= 0x80; //'behind' bit
    }

    for (const MovingObject& t : superTargets) {
        t.updatePPU(ppu);
        // if (i % 2)
        //     ppu.sprites[i].attributes |= 0x80; //'behind' bit
    }

    //--- actually draw ---
    ppu.draw(drawable_size);
}
