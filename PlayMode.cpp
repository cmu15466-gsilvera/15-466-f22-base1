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

    // main character
    {
        // load png
        glm::uvec2 size;
        std::vector<glm::u8vec4> data;
        OriginLocation origin = OriginLocation::UpperLeftOrigin;
        load_png("assets/siphon.png", &size, &data, origin);
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
        siphon.spriteID = 0;
        siphon.pos.x = PPU466::ScreenWidth / 2;
        siphon.pos.y = PPU466::ScreenHeight / 2;
        siphon.sprite.index = 32;
        siphon.sprite.attributes = 7;
        ppu.tile_table[siphon.sprite.index] = siphon_sd.GetBits();
        ppu.palette_table[siphon.sprite.attributes] = siphon_sd.colours;
    }

    // projectiles
    {
        projectiles.reserve(numProjectiles);
        const int proj_idx = 6;
        // colours used for the misc other sprites:
        ppu.palette_table[proj_idx] = {
            glm::u8vec4(0x00, 0x00, 0x00, 0x00),
            glm::u8vec4(0x88, 0x88, 0xff, 0xff),
            glm::u8vec4(0x00, 0x00, 0x00, 0xff),
            glm::u8vec4(0x00, 0x00, 0x00, 0x00),
        };
        for (size_t i = 0; i < numProjectiles; i++) {
            Projectile newProj;
            newProj.spriteID = i + 1;
            newProj.sprite.index = 32;
            newProj.sprite.attributes = proj_idx;
            newProj.randomInit();
            projectiles.push_back(newProj);
        }
    }

    // background
    {
        const int background_idx = 0;
        ppu.palette_table[background_idx] = {
            glm::u8vec4(0x55, 0x55, 0x55, 0xFF),
            glm::u8vec4(0x55, 0x55, 0x55, 0xFF),
            glm::u8vec4(0x55, 0x55, 0x55, 0xFF),
            glm::u8vec4(0x55, 0x55, 0x55, 0xFF),
        };

        for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
            for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
                ppu.background[x + PPU466::BackgroundWidth * y] = background_idx;
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
        ppu.tile_table[siphon.sprite.index] = siphon_sd.GetBits(2);
        siphon.aimDirection = 3;
    }
    if (aim_right.pressed) {
        ppu.tile_table[siphon.sprite.index] = siphon_sd.GetBits(0);
        siphon.aimDirection = 0;
    }
    if (aim_down.pressed) {
        ppu.tile_table[siphon.sprite.index] = siphon_sd.GetBits(1);
        siphon.aimDirection = 1;
    }
    if (aim_up.pressed) {
        ppu.tile_table[siphon.sprite.index] = siphon_sd.GetBits(3);
        siphon.aimDirection = 2;
    }

    siphon.pos += dt * siphon.vel;

    siphon.pos.x = std::max(1.f, std::min(float(PPU466::ScreenWidth - 8), siphon.pos.x));
    siphon.pos.y = std::max(1.f, std::min(float(PPU466::ScreenHeight - 8), siphon.pos.y));
}

void PlayMode::ProjectileUpdate(float dt)
{
    for (Projectile& p : projectiles) {
        p.pos += dt * p.vel;
        if (p.pos.x < 0 || p.pos.y < 0 || p.pos.x > PPU466::ScreenWidth || p.pos.y > PPU466::ScreenHeight) {
            p.randomInit();
        }
    }
}

void PlayMode::update(float dt)
{

    // slowly rotates through [0,1):
    //  (will be used to set background color)
    background_fade += dt / 10.0f;
    background_fade -= std::floor(background_fade);

    PlayerUpdate(dt);

    ProjectileUpdate(dt);
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
    // ppu.background_position.x = int32_t(-0.5f * siphon.x);
    // ppu.background_position.y = int32_t(-0.5f * siphon.y);

    // player sprite:
    siphon.sprite.x = siphon.pos.x;
    siphon.sprite.y = siphon.pos.y;
    ppu.sprites[siphon.spriteID] = siphon.sprite;

    // projectile sprites
    for (const Projectile& p : projectiles) {
        int i = p.spriteID;
        // float amt = (i + 2.0f * background_fade) / 62.0f;
        ppu.sprites[i].x = p.pos[0];
        ppu.sprites[i].y = p.pos[1];
        ppu.sprites[i].index = p.sprite.index;
        ppu.sprites[i].attributes = p.sprite.attributes;
        // if (i % 2)
        //     ppu.sprites[i].attributes |= 0x80; //'behind' bit
    }

    //--- actually draw ---
    ppu.draw(drawable_size);
}
