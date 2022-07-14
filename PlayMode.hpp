#include "Mode.hpp"
#include "PPU466.hpp"

#include <glm/glm.hpp>

#include <deque>
#include <vector>

struct Object {
    int spriteID;
    glm::vec2 pos, vel;
    PPU466::Sprite sprite;
};

struct Siphon : Object {
    int aimDirection = 0;
    const float speed = 50.f;
};

struct Projectile : Object {
    int wall;
    const float speed = 30.f;

    void randomInit()
    {
        wall = rand() % 4;
        if (wall == 0) { // right
            pos.x = PPU466::ScreenWidth;
            pos.y = rand() % PPU466::ScreenHeight;
            vel = speed * glm::vec2(-1, 0);
        } else if (wall == 1) { // bottom
            pos.y = 0;
            pos.x = rand() % PPU466::ScreenHeight;
            vel = speed * glm::vec2(0, 1);
        } else if (wall == 2) { // left
            pos.x = 0;
            pos.y = rand() % PPU466::ScreenHeight;
            vel = speed * glm::vec2(1, 0);
        } else { // top
            pos.x = rand() % PPU466::ScreenWidth;
            pos.y = PPU466::ScreenHeight;
            vel = speed * glm::vec2(0, -1);
        }
    }
};

struct PlayMode : Mode {
    PlayMode();
    virtual ~PlayMode();

    // functions called by main loop:
    virtual bool handle_event(SDL_Event const&, glm::uvec2 const& window_size) override;
    virtual void update(float elapsed) override;
    virtual void draw(glm::uvec2 const& drawable_size) override;

    //----- game state -----

    Siphon siphon;
    SpriteData siphon_sd;
    void PlayerUpdate(float dt);

    const int numProjectiles = 5;
    std::vector<Projectile> projectiles;
    void ProjectileUpdate(float dt);

    // input tracking:
    struct Button {
        uint8_t pressed;
    } left, right, down, up, aim_left, aim_right, aim_down, aim_up;

    std::vector<std::pair<Button&, int>> key_assignment = {
        { aim_left, SDLK_LEFT },
        { aim_right, SDLK_RIGHT },
        { aim_up, SDLK_UP },
        { aim_down, SDLK_DOWN },
        { left, SDLK_a },
        { right, SDLK_d },
        { up, SDLK_w },
        { down, SDLK_s },
    };

    // some weird background animation:
    float background_fade = 0.0f;

    //----- drawing handled by PPU466 -----

    PPU466 ppu;
};
