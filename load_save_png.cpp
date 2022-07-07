#include "load_save_png.hpp"

#include <png.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

#define LOG_ERROR(X) std::cerr << X << std::endl
#define COLOR_TO_VEC4(c) (glm::vec4(c.r, c.g, c.b, c.a))

using std::vector;

bool load_png(std::istream& from, unsigned int* width, unsigned int* height, vector<glm::u8vec4>* data, OriginLocation origin);
void save_png(std::ostream& to, unsigned int width, unsigned int height, glm::u8vec4 const* data, OriginLocation origin);

void load_png(std::string filename, glm::uvec2* size, std::vector<glm::u8vec4>* data, OriginLocation origin)
{
    assert(size);

    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open PNG image file '" + filename + "'.");
    }
    if (!load_png(file, &size->x, &size->y, data, origin)) {
        throw std::runtime_error("Failed to read PNG image from '" + filename + "'.");
    }
}

void save_png(std::string filename, glm::uvec2 size, glm::u8vec4 const* data, OriginLocation origin)
{
    std::ofstream file(filename.c_str(), std::ios::binary);
    save_png(file, size.x, size.y, data, origin);
}

static void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    std::istream* from = reinterpret_cast<std::istream*>(png_get_io_ptr(png_ptr));
    assert(from);
    if (!from->read(reinterpret_cast<char*>(data), length)) {
        png_error(png_ptr, "Error reading.");
    }
}

static void user_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    std::ostream* to = reinterpret_cast<std::ostream*>(png_get_io_ptr(png_ptr));
    assert(to);
    if (!to->write(reinterpret_cast<char*>(data), length)) {
        png_error(png_ptr, "Error writing.");
    }
}

static void user_flush_data(png_structp png_ptr)
{
    std::ostream* to = reinterpret_cast<std::ostream*>(png_get_io_ptr(png_ptr));
    assert(to);
    if (!to->flush()) {
        png_error(png_ptr, "Error flushing.");
    }
}

bool load_png(std::istream& from, unsigned int* width, unsigned int* height, vector<glm::u8vec4>* data, OriginLocation origin)
{
    assert(data);
    uint32_t local_width, local_height;
    if (width == nullptr)
        width = &local_width;
    if (height == nullptr)
        height = &local_height;
    *width = *height = 0;
    data->clear();
    //..... load file ......
    // Load a png file, as per the libpng docs:
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, (png_error_ptr)NULL, (png_error_ptr)NULL);

    png_set_read_fn(png, &from, user_read_data);

    if (!png) {
        LOG_ERROR("  cannot alloc read struct.");
        return false;
    }
    png_infop info = png_create_info_struct(png);
    if (!info) {
        LOG_ERROR("  cannot alloc info struct.");
        png_destroy_read_struct(&png, (png_infopp)NULL, (png_infopp)NULL);
        return false;
    }
    png_bytep* row_pointers = NULL;
    if (setjmp(png_jmpbuf(png))) {
        LOG_ERROR("  png interal error.");
        png_destroy_read_struct(&png, &info, (png_infopp)NULL);
        if (row_pointers != NULL)
            delete[] row_pointers;
        data->clear();
        return false;
    }
    // not needed with custom read/write functions: png_init_io(png, NULL);
    png_read_info(png, info);
    unsigned int w = png_get_image_width(png, info);
    unsigned int h = png_get_image_height(png, info);
    if (png_get_color_type(png, info) == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);
    if (png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY || png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);
    if (!(png_get_color_type(png, info) & PNG_COLOR_MASK_ALPHA))
        png_set_add_alpha(png, 0xff, PNG_FILLER_AFTER);
    if (png_get_bit_depth(png, info) < 8)
        png_set_packing(png);
    if (png_get_bit_depth(png, info) == 16)
        png_set_strip_16(png);
    // Ok, should be 32-bit RGBA now.

    png_read_update_info(png, info);
    size_t rowbytes = png_get_rowbytes(png, info);
    // Make sure it's the format we think it is...
    assert(rowbytes == w * sizeof(uint32_t));

    data->resize(w * h);
    row_pointers = new png_bytep[h];
    for (unsigned int r = 0; r < h; ++r) {
        if (origin == LowerLeftOrigin) {
            row_pointers[h - 1 - r] = (png_bytep)(&(*data)[r * w]);
        } else {
            row_pointers[r] = (png_bytep)(&(*data)[r * w]);
        }
    }
    png_read_image(png, row_pointers);
    png_destroy_read_struct(&png, &info, NULL);
    delete[] row_pointers;

    *width = w;
    *height = h;
    return true;
}

void save_png(std::ostream& to, unsigned int width, unsigned int height, glm::u8vec4 const* data, OriginLocation origin)
{
    // After the libpng example.c
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    png_set_write_fn(png_ptr, &to, user_write_data, user_flush_data);

    if (png_ptr == NULL) {
        LOG_ERROR("Can't create write struct.");
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_write_struct(&png_ptr, NULL);
        LOG_ERROR("Can't craete info pointer");
        return;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        LOG_ERROR("Error writing png.");
        return;
    }

    // Not needed with custom read/write functions: png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);
    // png_set_swap_alpha(png_ptr) // might need?
    vector<png_bytep> row_pointers(height);
    for (unsigned int i = 0; i < height; ++i) {
        if (origin == UpperLeftOrigin) {
            row_pointers[i] = (png_bytep) & (data[i * width]);
        } else {
            row_pointers[i] = (png_bytep) & (data[(height - 1 - i) * width]);
        }
    }
    png_write_image(png_ptr, &(row_pointers[0]));

    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    return;
}

glm::u8vec4 tile_avg(const std::vector<glm::u8vec4>& data, const glm::uvec2 size, const size_t x, const size_t y, const size_t w, const size_t h)
{
    // take the average of colours into one
    int r, g, b, a;
    r = g = b = a = 0;
    for (size_t j = 0; j < h; j++) {
        for (size_t i = 0; i < w; i++) {
            const glm::u8vec4& og_colour = data[((x * w) + i) + size.x * ((y * h) + j)];
            r += (int)(og_colour.r);
            g += (int)(og_colour.g);
            b += (int)(og_colour.b);
            a += (int)(og_colour.a);
        }
    }
    const size_t count = w * h;
    return glm::u8vec4(r / count, g / count, b / count, a / count);
}

glm::u8vec4 closest_in_bank(const glm::u8vec4& og_colour, const std::vector<glm::u8vec4>& colour_bank)
{
    // compute (euclidean) distance to the representative n colours and use the best colour (smallest dist)
    // to represent this pixel
    glm::u8vec4 new_colour = colour_bank[0];
    float best_dist = 1e9; // +inf
    for (const glm::u8vec4& nth_colour : colour_bank) {
        const float dist = glm::length(COLOR_TO_VEC4(nth_colour) - COLOR_TO_VEC4(og_colour));
        if (dist < best_dist) {
            best_dist = dist;
            new_colour = nth_colour;
        }
    }
    return new_colour;
}

void convert_to_n_colours(const size_t n, const glm::uvec2 size, glm::u8vec4* data, std::vector<glm::u8vec4>& bank)
{
    // convert the spectrum of colours from 0-255 to 0-n respecting the distribution
    // first determine which n colours are to be used

    bank = {
        glm::u8vec4(0, 0, 0, 0),
        glm::u8vec4(255, 0, 0, 255),
        glm::u8vec4(0, 255, 0, 255),
        glm::u8vec4(0, 0, 255, 255),
    };

    // then determine how these colours map to the data
    for (size_t y = 0; y < size.y; y++) {
        for (size_t x = 0; x < size.x; x++) {
            data[x + size.x * y] = closest_in_bank(data[x + size.x * y], bank);
        }
    }
}

void convert_to_new_size(const glm::uvec2 new_size, glm::uvec2& size, std::vector<glm::u8vec4>& data)
{
    // downsample the image from size.x x size.y to (new_size.x x new_size.y)
    /// NOTE: only supports downsampling currently (not upsampling!)
    std::vector<glm::u8vec4> new_data;
    const size_t iter_x = size.x / new_size.x;
    const size_t iter_y = size.y / new_size.y;
    /// TODO: check this is the best (loop traversal) order for cache locality
    for (size_t arr_y = 0; arr_y < new_size.y; arr_y++) {
        for (size_t arr_x = 0; arr_x < new_size.x; arr_x++) {
            new_data.push_back(tile_avg(data, size, arr_x, arr_y, iter_x, iter_y));
        }
    }
    data.clear();
    // update the original data with the new data
    data = new_data;
    size = new_size;
}

void convert_to_new_size_with_bank(const glm::uvec2 new_size, glm::uvec2& size, std::vector<glm::u8vec4>& data, const std::vector<glm::u8vec4>& colour_bank)
{
    // downsample the image from size.x x size.y to (new_size.x x new_size.y)
    /// NOTE: only supports downsampling currently (not upsampling!)
    std::vector<glm::u8vec4> new_data;
    const size_t iter_x = size.x / new_size.x;
    const size_t iter_y = size.y / new_size.y;
    /// TODO: check this is the best (loop traversal) order for cache locality
    for (size_t arr_y = 0; arr_y < new_size.y; arr_y++) {
        for (size_t arr_x = 0; arr_x < new_size.x; arr_x++) {
            glm::u8vec4 avg = tile_avg(data, size, arr_x, arr_y, iter_x, iter_y);
            new_data.push_back(closest_in_bank(avg, colour_bank));
        }
    }
    data.clear();
    // update the original data with the new data
    data = new_data;
    size = new_size;
}