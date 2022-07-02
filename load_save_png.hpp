#pragma once

#include <glm/glm.hpp>

#include <stdint.h>
#include <string>
#include <vector>

/*
 * Load and save PNG files.
 */

enum OriginLocation {
    LowerLeftOrigin,
    UpperLeftOrigin,
};

// NOTE: load_png will throw on error
void load_png(std::string filename, glm::uvec2* size, std::vector<glm::u8vec4>* data, OriginLocation origin);
void save_png(std::string filename, glm::uvec2 size, glm::u8vec4 const* data, OriginLocation origin);

/*
 * Conversion pipeline between standard PNG's and PPU466.
 */

void convert_to_n_colours(const size_t n, glm::uvec2 size, glm::u8vec4* data, std::vector<glm::u8vec4>& bank);
void convert_to_new_size(const glm::uvec2 new_size, glm::uvec2& size, std::vector<glm::u8vec4>& data);
void convert_to_new_size_with_bank(const glm::uvec2 new_size, glm::uvec2& size, std::vector<glm::u8vec4>& data, const std::vector<glm::u8vec4>& bank);