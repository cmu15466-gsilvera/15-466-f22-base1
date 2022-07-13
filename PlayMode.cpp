#include "PlayMode.hpp"

// for the GL_ERRORS() macro:
#include "gl_errors.hpp"

// for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

// load_save_png
#include "load_save_png.hpp"

#include <algorithm> // std::clamp
#include <random>

PPU466::Sprite Projectile::sprite;

PlayMode::PlayMode()
{

    {
        // load png
        glm::uvec2 size;
        std::vector<glm::u8vec4> data;
        OriginLocation origin = OriginLocation::UpperLeftOrigin;
        load_png("assets/mario.png", &size, &data, origin);
        std::vector<glm::u8vec4> colour_bank;
        convert_to_n_colours(4, size, &(data[0]), colour_bank);
        convert_to_new_size_with_bank(glm::uvec2(8, 8), size, data, colour_bank);
        save_png("assets/cato-saved.png", size, &(data[0]), origin);

        SpriteData player_sprite(data, colour_bank);

        // initialize siphon (player) data
        siphon.x = PPU466::ScreenWidth / 2;
        siphon.y = PPU466::ScreenHeight / 2;
        siphon.index = 32;
        siphon.attributes = 7;

        // use sprite 32 as a "player":
        ppu.tile_table[siphon.index] = player_sprite.bits;
        ppu.palette_table[7] = player_sprite.colours;
    }

    {
        Projectile::sprite.index = 32;
        Projectile::sprite.attributes = 6;
        projectiles.reserve(numProjectiles);
        for (size_t i = 0; i < numProjectiles; i++) {
            Projectile newProj;
            newProj.spriteIdx = i + 1;
            newProj.randomInit();
            projectiles.push_back(newProj);
        }
    }

    // makes the outside of tiles 0-16 solid:
    ppu.palette_table[0] = {
        glm::u8vec4(0x00, 0x00, 0x00, 0x00),
        glm::u8vec4(0x00, 0x00, 0x00, 0xff),
        glm::u8vec4(0x00, 0x00, 0x00, 0x00),
        glm::u8vec4(0x00, 0x00, 0x00, 0xff),
    };

    // makes the center of tiles 0-16 solid:
    ppu.palette_table[1] = {
        glm::u8vec4(0x00, 0x00, 0x00, 0x00),
        glm::u8vec4(0x00, 0x00, 0x00, 0x00),
        glm::u8vec4(0x00, 0x00, 0x00, 0xff),
        glm::u8vec4(0x00, 0x00, 0x00, 0xff),
    };

    // colours used for the misc other sprites:
    ppu.palette_table[6] = {
        glm::u8vec4(0x00, 0x00, 0x00, 0x00),
        glm::u8vec4(0x88, 0x88, 0xff, 0xff),
        glm::u8vec4(0x00, 0x00, 0x00, 0xff),
        glm::u8vec4(0x00, 0x00, 0x00, 0x00),
    };
}

PlayMode::~PlayMode()
{
}

bool PlayMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size)
{

    if (evt.type == SDL_KEYDOWN) {
        if (evt.key.keysym.sym == SDLK_LEFT) {
            left.downs += 1;
            left.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_RIGHT) {
            right.downs += 1;
            right.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_UP) {
            up.downs += 1;
            up.pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_DOWN) {
            down.downs += 1;
            down.pressed = true;
            return true;
        }
    } else if (evt.type == SDL_KEYUP) {
        if (evt.key.keysym.sym == SDLK_LEFT) {
            left.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_RIGHT) {
            right.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_UP) {
            up.pressed = false;
            return true;
        } else if (evt.key.keysym.sym == SDLK_DOWN) {
            down.pressed = false;
            return true;
        }
    }

    return false;
}

void PlayMode::update(float dt)
{

    // slowly rotates through [0,1):
    //  (will be used to set background color)
    background_fade += dt / 10.0f;
    background_fade -= std::floor(background_fade);

    constexpr float speed = 1.0f;
    if (left.pressed)
        siphon.x -= speed;
    if (right.pressed)
        siphon.x += speed;
    if (down.pressed)
        siphon.y -= speed;
    if (up.pressed)
        siphon.y += speed;

    siphon.x = std::max(uint8_t(1), std::min(uint8_t(PPU466::ScreenWidth - 8), siphon.x));
    siphon.y = std::max(uint8_t(1), std::min(uint8_t(PPU466::ScreenHeight - 8), siphon.y));

    // reset button press counters:
    left.downs = 0;
    right.downs = 0;
    up.downs = 0;
    down.downs = 0;

    for (Projectile& p : projectiles) {
        p.pos += p.vel;
        if (p.pos.x < 0 || p.pos.y < 0 || p.pos.x > PPU466::ScreenWidth || p.pos.y > PPU466::ScreenHeight) {
            p.randomInit();
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

    // tilemap gets recomputed every frame as some weird plasma thing:
    // NOTE: don't do this in your game! actually make a map or something :-)
    for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
        for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
            // TODO: make weird plasma thing
            ppu.background[x + PPU466::BackgroundWidth * y] = ((x + y) % 16);
        }
    }

    // background scroll:
    ppu.background_position.x = int32_t(-0.5f * siphon.x);
    ppu.background_position.y = int32_t(-0.5f * siphon.y);

    // player sprite:
    ppu.sprites[0] = siphon;

    // some other misc sprites:
    // for (uint32_t i = 0; i < projectile_pos.size(); ++i) {
    for (const Projectile& p : projectiles) {
        int i = p.spriteIdx;
        // float amt = (i + 2.0f * background_fade) / 62.0f;
        ppu.sprites[i].x = p.pos[0];
        ppu.sprites[i].y = p.pos[1];
        ppu.sprites[i].index = p.sprite.index;
        ppu.sprites[i].attributes = p.sprite.attributes;
        if (i % 2)
            ppu.sprites[i].attributes |= 0x80; //'behind' bit
    }

    //--- actually draw ---
    ppu.draw(drawable_size);
}
