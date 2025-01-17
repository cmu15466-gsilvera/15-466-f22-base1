#include "Mode.hpp"
#include "PPU466.hpp"

#include <glm/glm.hpp>

#include <deque>
#include <vector>

// colour indexes
#define CLEAR_COLOUR 0
#define BACKGROUND_COLOUR 1
#define PROJECTILE_COLOUR 4
#define TARGET_COLOUR 5
#define SUPER_TARGET_COLOUR 6
#define SIPHON_COLOUR 7

// sprite indexes
#define SIPHON_SPRITE_IDX 32
#define PROJECTILE_SPRITE_IDX_0 33
#define PROJECTILE_SPRITE_IDX_1 34
#define TARGET_SPRITE_IDX 35

struct Object {
    int spriteID;
    glm::vec2 pos, vel;
    PPU466::Sprite sprite;
    bool bIsEnabled = true;
    float speed = 30.f;
    float hiddenDuration = 0.f;

    void updatePPU(PPU466& ppu) const
    {
        ppu.sprites[spriteID].x = pos[0];
        ppu.sprites[spriteID].y = pos[1];
        ppu.sprites[spriteID].index = sprite.index;
        if (bIsEnabled) {
            ppu.sprites[spriteID].attributes = sprite.attributes;
        } else {
            ppu.sprites[spriteID].attributes = CLEAR_COLOUR;
        }
    }

    bool collisionWith(const Object& other) const
    {
        if (!(bIsEnabled && other.bIsEnabled)) {
            return false;
        }
        const glm::vec2& otherPos = other.pos;
        auto isPtIn = [this](const glm::vec2& pt) {
            bool x_check = (pt.x > this->pos.x - 4 && pt.x < this->pos.x + 4);
            bool y_check = (pt.y > this->pos.y - 4 && pt.y < this->pos.y + 4);
            return x_check && y_check;
        };
        // check if the four corners (and the center) are within this sprite area
        glm::vec2 topLeft = otherPos + glm::vec2(-4, 4);
        glm::vec2 bottomLeft = otherPos + glm::vec2(-4, -4);
        glm::vec2 topRight = otherPos + glm::vec2(4, 4);
        glm::vec2 bottomRight = otherPos + glm::vec2(4, -4);
        glm::vec2 center = otherPos; // if both sprites perfectly overlap then the corner float cmp's won't work reliably
        return isPtIn(center) || isPtIn(topLeft) || isPtIn(bottomLeft) || isPtIn(topRight) || isPtIn(bottomRight);
    }

    bool atEdge() const
    {
        return (pos.x < 0 || pos.y < 0 || pos.x > PPU466::ScreenWidth || pos.y > PPU466::ScreenHeight);
    }

    void update(float dt)
    {
        pos += dt * vel;
        hiddenDuration -= dt;
        if (hiddenDuration > 0) {
            pos = glm::vec2(-1, -1); // trigger a reinit after unhidden
            bIsEnabled = false;
        } else {
            bIsEnabled = true;
        }
    }

    void hide(float duration)
    {
        hiddenDuration = duration;
    }
};

struct Siphon : Object {
    int aimDirection = 0;
};

struct MovingObject : Object {
    int wall;
    bool collision = false;

    static glm::vec2 directionMapping(int direction)
    {
        if (direction == 0) { // right
            return glm::vec2(1, 0);
        } else if (direction == 1) { // bottom
            return glm::vec2(0, -1);
        } else if (direction == 2) { // left
            return glm::vec2(-1, 0);
        }
        return glm::vec2(0, 1); // up
    }

    void update(float dt)
    {
        Object::update(dt);
        // reinitialize the location once they reach the edge
        if (bIsEnabled && atEdge()) {
            randomInit();
        }
    }

    void randomInit()
    {
        wall = rand() % 4;
        if (wall == 0) { // right
            pos.x = PPU466::ScreenWidth - 8;
            pos.y = rand() % PPU466::ScreenHeight - 8;
            vel = -speed * directionMapping(0);
        } else if (wall == 1) { // bottom
            pos.y = 8;
            pos.x = rand() % PPU466::ScreenHeight - 8;
            vel = -speed * directionMapping(1);
        } else if (wall == 2) { // left
            pos.x = 8;
            pos.y = rand() % PPU466::ScreenHeight - 8;
            vel = -speed * directionMapping(2);
        } else { // top
            pos.x = rand() % PPU466::ScreenWidth - 8;
            pos.y = PPU466::ScreenHeight - 8;
            vel = -speed * directionMapping(3);
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
    int score = 0;
    float time_left = 30.0f; // number of seconds you have to play the game
    bool end_msg = false;

    Siphon siphon;
    SpriteData siphon_sd;
    void PlayerUpdate(float dt);

    const int numProjectiles = 5;
    std::vector<MovingObject> projectiles;
    void ProjectileUpdate(float dt);

    const int numTargets = 3;
    std::vector<MovingObject> targets;
    void TargetsUpdate(float dt);

    const int numSuperTargets = 1;
    std::vector<MovingObject> superTargets;

    // input tracking:
    struct Button {
        uint8_t pressed = 0;
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
