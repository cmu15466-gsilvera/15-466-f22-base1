# Redirekt

Author: Gustavo Silvera

Design: 2D "bullet-hell" style game where the objective is to maximize your score in a set amount of time by redirecting projectiles (in the 4 cardinal directions) to hit targets. 

# Screen Shot:

![Screen Shot](screenshot.png)

# How The Asset Pipeline Works:

1. I will create a sprite using a pixel-art generator such as [pixilart.com](https://www.pixilart.com/draw) and only use four (4) RGBA colours. These are saved in `assets` as [`bolt.png`](assets/bolt.png), [`siphon.png`](assets/siphon.png), and [`target.png`](assets/target.png).
2. On initialization the game will load these `png`s with `load_png` to create the small array in memory.
3. Alongside hardcoded colour banks (that may or may not match the colours in the `png`s) per sprite, the data will be sent through `convert_to_new_size_with_bank` which downsamples the image to the given size (8x8) and assigns the colours from the colour bank to the `closest_in_bank` which takes the source (`png`) pixel colour and computes the euclidean distance to each of the (4) colours in the bank to find the "best fit".
4. Once the appropriate `data` is filled (after `convert_to_new_size_with_bank`) it is passed to the constructor of a custom class `SpriteData` which holds the bits and colour palette and converts the `std::vector<glm::u8vec4> data` array into the appropriate bitmap. 
5. [OPTIONAL] As an optional sprite, there is also functionality to `rotate90CW` the bits in the bitmap which enables the same sprite (with the same colours) to be displayed in various rotations (all cardinal directions) without redrawing. This is useful for projectiles (lightning bolts) which are "rotated" depending on their direction. 

# Custom Sprites

| lightning bolt | siphon | target |
| --- | --- | --- |
| ![`bolt.png`](assets/bolt.png) | ![`siphon.png`](assets/siphon.png) | ![`target.png`](assets/target.png) |

# How To Play:

The goal of this game is to use the siphon (gray & red player sprite) to redirect the lightning bolts to hit the targets (red and purple) and maximize your score in the alloted time period (30 seconds). 

You can use the arrow keys (`Up`, `Down`, `Left`, `Right`) to change the heading of the siphon (direction of redirection). The heading is visually represented by the red "tube" indicating the forward (output) direction.

You can also use `WASD` to move the siphon (player) around and when it collides with a projectile (lightning bolt) it will redirect the projectile depending on the direction of the siphon. 

Note that the player is the fastest object in the game so you can redirect the projectiles several times. 

This game was built with [NEST](NEST.md).

