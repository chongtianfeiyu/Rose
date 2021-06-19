/* $Id: sdl_utils.hpp 47608 2010-11-21 01:56:29Z shadowmaster $ */
/*
   Copyright (C) 2003 - 2010 by David White <dave@whitevine.net>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/** @file */

#ifndef SDL_UTILS_INCLUDED
#define SDL_UTILS_INCLUDED

#include "util.hpp"
#include "serialization/string_utils.hpp"
#include "wml_exception.hpp"

#include "SDL.h"
#include "SDL_render.h"
#include "sdl_rotate.h"

// <opencv\opencv2\core\cvdef.h(73,10): fatal error C1083: Cannot open include file: 'cvconfig.h': No such file or directory
// #include <opencv2/core/mat.hpp>
#include <opencv2/core/mat.hpp>
// #include <opencv2/core.hpp>

#include <cstdlib>
#include <iosfwd>
#include <map>
#include <string>


#define FINGER_HIT_THRESHOLD		4
#define FINGER_MOTION_THRESHOLD		10

#define MOUSE_HIT_THRESHOLD			0
#define MOUSE_MOTION_THRESHOLD		1

#define NO_MODULATE_ALPHA	255

extern const SDL_Rect empty_rect;
extern const SDL_Rect null_rect;

#define is_empty_rect(rect)		(!(rect).w || !(rect).h)
#define is_null_rect(rect)		((rect).w < 0 || (rect).h < 0)

SDL_Keycode sdl_keysym_from_name(std::string const &keyname);

bool point_in_rect(int x, int y, const SDL_Rect& rect);
bool rects_overlap(const SDL_Rect& rect1, const SDL_Rect& rect2);
SDL_Rect intersect_rects(SDL_Rect const &rect1, SDL_Rect const &rect2);
SDL_Rect union_rects(const SDL_Rect &rect1, const SDL_Rect &rect2);

/**
 *  Creates an empty SDL_Rect.
 *
 *  Since SDL_Rect doesn't have a constructor it's not possible to create it as
 *  a temporary for a function parameter. This functions overcomes this limit.
 */
SDL_Rect create_rect(const int x, const int y, const int w, const int h);
SDL_Point create_point(const int x, const int y);

namespace cv {
class Mat;
}

struct surface
{
private:
	static void sdl_add_ref(SDL_Surface *surf);

public:
	surface()
		: surface_(nullptr)
		, mat_(nullptr)
	{}

	surface(SDL_Surface *surf) 
		: surface_(surf)
		, mat_(nullptr)
	{}

	surface(const cv::Mat& mat);

	surface(const surface& o)
		: surface_(o.surface_)
		, mat_(o.mat_)
	{
		sdl_add_ref(surface_);
	}

	~surface();

	void assign(const surface& o)
	{
		SDL_Surface* surf = o.surface_;
		sdl_add_ref(surf); // need to be done before assign to avoid corruption on "a=a;"
		assign2(surf, o.mat_);
	}

	surface& operator=(const surface& o)
	{
		assign(o);
		return *this;
	}

	operator SDL_Surface*() const { return surface_; }

	SDL_Surface* get() const { return surface_; }

	SDL_Surface* operator->() const { return surface_; }

	bool null() const { return surface_ == nullptr; }

private:
	void assign2(SDL_Surface* surf, cv::Mat* mat);
	void free_sdl_surface();

private:
	SDL_Surface* surface_;
	cv::Mat* mat_;
};

bool operator<(const surface& a, const surface& b);

// SDL_Texture hasn't refcount member, use std::shared_ptr.
struct texture: public std::shared_ptr<SDL_Texture>
{
	texture()
		: std::shared_ptr<SDL_Texture>()
	{}

	texture(SDL_Texture* tex)
		: std::shared_ptr<SDL_Texture>(tex, SDL_DestroyTexture)
	{}

	void reset(SDL_Texture* tex) { std::shared_ptr<SDL_Texture>::reset(tex, SDL_DestroyTexture); }
};

inline void sdl_blit(const surface& src, const SDL_Rect* src_rect, surface& dst, SDL_Rect* dst_rect){
	SDL_BlitSurface(src, src_rect, dst, dst_rect);
}

inline void sdl_fill_rect(surface& dst, SDL_Rect* dst_rect, const Uint32 color){
	SDL_FillRect(dst, dst_rect, color);
}

void render_line(SDL_Renderer* renderer, Uint32 argb, int x1, int y1, int x2, int y2);
void render_rect_frame(SDL_Renderer* renderer, const SDL_Rect& rect, Uint32 argb, int thickness);
void render_rect(SDL_Renderer* renderer, const SDL_Rect& rect, Uint32 argb);

/**
 * Check that the surface is neutral bpp 32.
 *
 * The surface may have an empty alpha channel.
 *
 * @param surf                    The surface to test.
 *
 * @returns                       The status @c true if neutral, @c false if not.
 */
bool is_neutral_surface(const surface& surf);

const SDL_PixelFormat& get_neutral_pixel_format();
surface create_neutral_surface(int w, int h, bool use_rle = true);
surface makesure_neutral_surface(const surface& surf);
surface clone_surface(const surface& surf);
void fill_surface(surface& surf, uint32_t color);
SDL_Rect calculate_max_foreground_region(const surface& surf, uint32_t background);

texture create_neutral_texture(const int w, const int h, const int access);
// I cannot use render_rect(renderer, dstrect, 0x80000000) to get alpha-mask effect, so use below.
texture create_alpha_texture(int width, int height, uint32_t color);

/**
 * Stretches a surface in the horizontal direction.
 *
 *  The stretches a surface it uses the first pixel in the horizontal
 *  direction of the original surface and copies that to the destination.
 *  This means only the first column of the original is used for the destination.
 *  @param surf              The source surface.
 *  @param w                 The width of the resulting surface.
 *  @param optimize          Should the return surface be RLE optimized.
 *
 *  @return                  An optimized surface.
 *                           returned.
 *  @retval 0                Returned upon error.
 *  @retval surf             Returned if w == surf->w, note this ignores the
 *                           optimize flag.
 */
surface stretch_surface_horizontal(const surface& surf, const unsigned w, int pixels);

/**
 *  Stretches a surface in the vertical direction.
 *
 *  The stretches a surface it uses the first pixel in the vertical
 *  direction of the original surface and copies that to the destination.
 *  This means only the first row of the original is used for the destination.
 *  @param surf              The source surface.
 *  @param h                 The height of the resulting surface.
 *  @param optimize          Should the return surface be RLE optimized.
 *
 *  @return                  An optimized surface.
 *                           returned.
 *
 *  @retval surf             Returned if h == surf->h, note this ignores the
 *                           optimize flag.
 */
surface stretch_surface_vertical(const surface& surf, const unsigned h, int pixels);

tpoint calculate_adaption_ratio_size(const int outer_w, const int outer_h, const int inner_w, const int inner_h);
surface get_adaption_ratio_surface(const surface& src, const int outer_width, const int outer_height);
cv::Mat get_adaption_ratio_mat(const cv::Mat& src, const int outer_width, const int outer_height);
tpoint calculate_adaption_ratio_size_cut(const int outer_w, const int outer_h, const int inner_w, const int inner_h);
void erase_isolated_pixel(cv::Mat& src, const int font_gray, const int background_gray);
uint32_t surf_calculate_most_color(const surface& surf);
double calculate_surface_blur(const surface& surf, const cv::Rect& roi);

/** Scale a surface
 *  @param surf              The source surface.
 *  @param w                 The width of the resulting surface.
 *  @param h                 The height of the resulting surface.
 *  @param optimize          Should the return surface be RLE optimized.
 *  @return                  A surface containing the scaled version of the source.
 *  @retval 0                Returned upon error.
 *  @retval surf             Returned if w == surf->w and h == surf->h
 *                           note this ignores the optimize flag.
 */
surface scale_surface(const surface &surf, int w, int h);
surface scale_surface_blended(const surface &surf, int w, int h, bool optimize=true);

surface rotate_landscape_anticolockwise90(const surface& surf);

surface adjust_surface_color(const surface &surf, int r, int g, int b, bool optimize=true);
void adjust_surface_color2(surface &surf, int red, int green, int blue);
surface greyscale_image(const surface &surf, bool optimize=true);
/** create an heavy shadow of the image, by blurring, increasing alpha and darkening */
surface shadow_image(const surface &surf, bool optimize=true);

/**
 * Recolors a surface using a map with source and converted palette values.
 * This is most often used for team-coloring.
 *
 * @param surf               The source surface.
 * @param map_rgb            Map of color values, with the keys corresponding to the
 *                           source palette, and the values to the recolored palette.
 * @param optimize           Whether the new surface should be RLE encoded. Only
 *                           useful when the source is not the screen and it is
 *                           going to be used multiple times.
 * @return                   A recolored surface, or a null surface if there are
 *                           problems with the source.
 */
surface recolor_image(surface surf, const std::map<Uint32, Uint32>& map_rgb,
	bool optimize=true);

surface brighten_image(const surface &surf, fixed_t amount, bool optimize=true);

/** Get a portion of the screen.
 *  Send NULL if the portion is outside of the screen.
 *  @param surf              The source surface.
 *  @param rect              The portion of the source surface to copy.
 *  @param optimize_format   Optimize by converting to result to display format.
 *                           Only useful if the source is not the screen and you
 *                           plan to blit the result on screen several times.
 *  @return                  A surface containing the portion of the source.
 *                           No RLE or Alpha bits are set.
 *  @retval 0                if error or the portion is outside of the surface.
 */
surface get_surface_portion(const texture& tex, SDL_Rect &rect);
surface get_surface_portion2(const surface& surf, SDL_Rect &rect);

surface adjust_surface_alpha(const surface& surf, fixed_t amount);
void adjust_surface_rect_alpha2(surface& surf, fixed_t amount, const SDL_Rect& rect, bool inside);
surface adjust_surface_alpha_add(const surface& surf, int amount, bool optimize=true);

/** Applies a mask on a surface. */
surface mask_surface(const surface &surf, const surface &mask, bool* empty_result = NULL);

/** Check if a surface fit into a mask */
bool in_mask_surface(const surface &surf, const surface &mask);

bool has_alpha_le(const surface& surf, uint8_t threshold);

/** Progressively reduce alpha of bottom part of the surface
 *  @param surf              The source surface.
 *  @param depth             The height of the bottom part in pixels
 *  @param alpha_base        The alpha adjustement at the interface
 *  @param alpha_delta       The alpha adjustement reduction rate by pixel depth
 *  @param optimize_format   Optimize by converting to result to display
*/
surface submerge_alpha(const surface &surf, int depth, float alpha_base, float alpha_delta, bool optimize=true);

/** Light surf using lightmap (RGB=128,128,128 means no change) */
surface light_surface(const surface &surf, const surface &lightmap, bool optimize=true);

/** Cross-fades a surface. */
surface blur_surface(const surface &surf, int depth = 1, bool optimize=true);

/**
 * Cross-fades a surface in place.
 *
 * @param surf                    The surface to blur, must be not optimized
 *                                and have 32 bits per pixel.
 * @param rect                    The part of the surface to blur.
 * @param depth                   The depth of the blurring.
 */
void blur_surface(surface& surf, SDL_Rect rect, int depth = 1);

/**
 * Cross-fades a surface with alpha channel.
 *
 * @todo FIXME: This is just an adapted copy-paste
 * of the normal blur but with blur alpha channel too
 */
surface blur_alpha_surface(const surface &surf, int depth = 1, bool optimize=true);

/** Cuts a rectangle from a surface. */
surface cut_surface(const surface &surf, SDL_Rect const &r);
surface blend_surface(const surface &surf, double amount, Uint32 color, bool optimize=true);
surface flip_surface(const surface &surf, bool optimize=true);
surface flop_surface(const surface &surf, bool optimize=true);
surface rotate_surface(const surface& surf, double angle);
surface rotate_surface2(const surface& surf, int srcx, int srcy, int degree, int offsetx, int offsety, int& dstx, int& dsty);
surface create_compatible_surface(const surface &surf, int width = -1, int height = -1);

void render_surface(SDL_Renderer* renderer, const surface& surf, const SDL_Rect* srcrect, const SDL_Rect* dstrect);
void texture_from_texture(const texture& src, texture& dst, const SDL_Rect* srcrect, const int dstwidth, const int dstheight);
texture clone_texture(const texture& src, const uint8_t r = NO_MODULATE_ALPHA, const uint8_t g = NO_MODULATE_ALPHA, const uint8_t b = NO_MODULATE_ALPHA);
void brighten_texture(const texture& tex, const uint8_t r, const uint8_t g, const uint8_t b);
void brighten_renderer(SDL_Renderer* renderer, const uint8_t r, const uint8_t g, const uint8_t b, const SDL_Rect* dstrect);
void grayscale_renderer(SDL_Renderer* renderer);
texture target_texture_from_surface(const surface& surf);
void imwrite(const texture& tex, const std::string& path);
void imwrite(const surface& surf, const std::string& path);
surface save_texture_to_surface(const texture& tex);

enum {img_png, img_jpg};
uint8_t* imwrite_mem(const surface& surf, int format, int* len_ptr);
surface imread_mem(const void* data, int size);

void fill_rect_alpha(SDL_Rect &rect, Uint32 color, Uint8 alpha, surface &target);

SDL_Rect get_non_transparent_portion(const surface &surf);

bool operator==(const SDL_Rect& a, const SDL_Rect& b);
bool operator!=(const SDL_Rect& a, const SDL_Rect& b);

bool operator==(const SDL_Color& a, const SDL_Color& b);
bool operator!=(const SDL_Color& a, const SDL_Color& b);
SDL_Color inverse(const SDL_Color& color);

#define FORMULA_COLOR		0x1 // 0 indicate no color.
#define PREDEFINE_COLOR		0x2
uint32_t decode_color(const std::string& color);
std::string encode_color(const uint32_t argb);

SDL_Color uint32_to_color(const Uint32 argb);
uint32_t color_to_uint32(const SDL_Color& color);

SDL_Color create_color(const unsigned char red
		, unsigned char green
		, unsigned char blue
		, unsigned char unused = 255);

/***** ***** ***** ***** ***** DRAWING PRIMITIVES ***** ***** ***** ***** *****/

/**
 * Draws a single pixel on a surface.
 *
 * @pre                   The caller needs to make sure the selected coordinate
 *                        fits on the @p surface.
 * @pre                   The @p canvas is locked.
 *
 * @param start           The memory address which is the start of the surface
 *                        buffer to draw in.
 * @param color           The color of the pixel to draw.
 * @param w               The width of the surface.
 * @param x               The x coordinate of the pixel to draw.
 * @param y               The y coordinate of the pixel to draw.
 */
void put_pixel(
		  const ptrdiff_t start
		, const Uint32 color
		, const unsigned w
		, const unsigned x
		, const unsigned y);

/**
 * Draws a line on a surface.
 *
 * @pre                   The caller needs to make sure the entire line fits on
 *                        the @p surface.
 * @pre                   @p x2 >= @p x1
 * @pre                   The @p surface is locked.
 *
 * @param canvas          The canvas to draw upon, the caller should lock the
 *                        surface before calling.
 * @param color           The color of the line to draw.
 * @param x1              The start x coordinate of the line to draw.
 * @param y1              The start y coordinate of the line to draw.
 * @param x2              The end x coordinate of the line to draw.
 * @param y2              The end y coordinate of the line to draw.
 */
void draw_line(
		  surface& canvas
		, Uint32 color
		, int x1
		, int y1
		, int x2
		, int y2);

/**
 * Draws a circle on a surface.
 *
 * @pre                   The circle must fit on the canvas.
 * @pre                   The @p surface is locked.
 *
 * @param canvas          The canvas to draw upon, the caller should lock the
 *                        surface before calling.
 * @param color           The color of the circle to draw.
 * @param x_centre        The x coordinate of the centre of the circle to draw.
 * @param y_centre        The y coordinate of the centre of the circle to draw.
 * @param radius          The radius of the circle to draw.
 */
void draw_circle(
		  surface& canvas
		, Uint32 color
		, const unsigned x_centre
		, const unsigned y_centre
		, const unsigned radius
		, bool require_map);

/**
 * Helper class for pinning SDL surfaces into memory.
 * @note This class should be used only with neutral surfaces, so that
 *       the pointer returned by #pixels is meaningful.
 */
struct surface_lock
{
	surface_lock(surface &surf);
	~surface_lock();

	Uint32* pixels() { return reinterpret_cast<Uint32*>(surface_->pixels); }
private:
	surface& surface_;
	bool locked_;
};

struct const_surface_lock
{
	const_surface_lock(const surface &surf);
	~const_surface_lock();

	const Uint32* pixels() const { return reinterpret_cast<const Uint32*>(surface_->pixels); }
private:
	const surface& surface_;
	bool locked_;
};

struct tsurface_2_mat_lock
{
	tsurface_2_mat_lock(const surface &surf);
	~tsurface_2_mat_lock();

	cv::Mat mat;

private:
	const surface& surface_;
	bool locked_;
};

struct ttexture_2_mat_lock
{
	ttexture_2_mat_lock(texture &tex);

	cv::Mat mat;

private:
	texture& texture_;
	bool locked_;
};

struct tframe_ticks {
	int frames;
	uint32_t task1;
	uint32_t task2;
	uint32_t task3;
	uint32_t task4;
};
extern tframe_ticks frame_ticks;

void cvtColor2(const cv::Mat& _src, cv::Mat& _dst, int code);

void draw_rectangle(int x, int y, int w, int h, Uint32 color, surface tg);
void draw_distinguish_rectangle(const SDL_Rect& rect, int long_size, int short_size, uint32_t color, surface target);

// blit the image on the center of the rectangle
// and a add a colored background
void draw_centered_on_background(surface surf, const SDL_Rect& rect,
	const SDL_Color& color, surface target);

void blit_integer_surface(int integer, surface& to, int x, int y);
surface generate_integer_surface(int integer);
surface generate_pip_surface(surface& bg, surface& fg);
surface generate_pip_surface(int width, int height, const std::string& bg, const std::string& fg);
surface generate_surface(int width, int height, const std::string& img, int integer, bool greyscale);

struct tarc_pixel {
	unsigned short x;
	unsigned short y;
	unsigned short degree;
};
tarc_pixel* circle_calculate_pixels(surface& surf, int* valid_pixels);
void circle_draw_arc(surface& surf, tarc_pixel* circle_pixels, const int valid_pixels, int start, int stop, Uint32 erase_col);

// Calculate the angle of OA to OB, angle value is between 0 and 180. 
// clockwise indicates the direction of rotation. 
double calculate_line_angle(bool image_view, const SDL_Point& O, const SDL_Point& A, const SDL_Point& B, bool& clockwise);
cv::Mat rotate_mat(const cv::Mat& src, double rotationAngle);
SDL_Rect enlarge_rect(const SDL_Rect& src, int left, int right, int top, int bottom, int max_width, int max_height);

std::ostream& operator<<(std::ostream& s, const SDL_Rect& rect);

class tsurface_blend_none_lock
{
public:
	tsurface_blend_none_lock(const surface& surf)
		: surf_(surf)
	{
		SDL_GetSurfaceBlendMode(surf, &original_);
		if (original_ != SDL_BLENDMODE_NONE) {
			SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_NONE);
		}
	}
	~tsurface_blend_none_lock()
	{
		if (original_ != SDL_BLENDMODE_NONE) {
			SDL_SetSurfaceBlendMode(surf_, original_);
		}
	}

private:
	const surface& surf_;
	SDL_BlendMode original_;
};

class ttexture_blend_none_lock
{
public:
	ttexture_blend_none_lock(const texture& tex)
		: tex_(tex)
	{
		SDL_GetTextureBlendMode(tex.get(), &original_);
		if (original_ != SDL_BLENDMODE_NONE) {
			SDL_SetTextureBlendMode(tex.get(), SDL_BLENDMODE_NONE);
		}
	}
	~ttexture_blend_none_lock()
	{
		if (original_ != SDL_BLENDMODE_NONE) {
			SDL_SetTextureBlendMode(tex_.get(), original_);
		}
	}

private:
	const texture& tex_;
	SDL_BlendMode original_;
};

SDL_Renderer* get_renderer();

class trender_target_lock
{
public:
	trender_target_lock(SDL_Renderer* renderer, const texture& target)
		: renderer_(renderer)
		, original_(SDL_GetRenderTarget(renderer))
		, render_pause_(false)
	{
		// if desire to back-fb, tex is NULL.
		SDL_Texture* tex = target.get();
		
		if (tex) {
			// if want texture to target, this texture must be SDL_TEXTUREACCESS_TARGET.
			int access;
			SDL_QueryTexture(tex, NULL, &access, NULL, NULL);
			VALIDATE(access == SDL_TEXTUREACCESS_TARGET, null_str);
		}

		int result = SDL_SetRenderTarget(renderer, tex);
		if (result != 0) {
			render_pause_ = true;
			SDL_SetRenderAppPause(renderer, SDL_TRUE);
		}
	}
	~trender_target_lock()
	{
		if (!render_pause_) {
			SDL_SetRenderTarget(renderer_, original_);
		} else {
			SDL_SetRenderAppPause(renderer_, SDL_FALSE);
		}
	}

private:
	SDL_Renderer* renderer_;
	SDL_Texture* original_;
	bool render_pause_;
};

class trender_draw_color_lock
{
public:
	trender_draw_color_lock(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
		: renderer_(renderer)
	{
		SDL_GetRenderDrawColor(renderer, &original_r_, &original_g_, &original_b_, &original_a_);
		SDL_SetRenderDrawColor(renderer, r, g, b, a);
	}
	~trender_draw_color_lock()
	{
		SDL_SetRenderDrawColor(renderer_, original_r_, original_g_, original_b_, original_a_);
	}

private:
	SDL_Renderer* renderer_;
	Uint8 original_r_;
	Uint8 original_g_;
	Uint8 original_b_;
	Uint8 original_a_;
};

struct clip_rect_setter
{
	// if r is NULL, clip to the full size of the surface.
	clip_rect_setter(const surface &surf, const SDL_Rect* r, bool operate = true) : surface_(surf), rect_(), operate_(operate)
	{
		if(operate_){
			SDL_GetClipRect(surface_, &rect_);
			SDL_SetClipRect(surface_, r);
		}
	}

	~clip_rect_setter() {
		if (operate_)
			SDL_SetClipRect(surface_, &rect_);
	}

private:
	surface surface_;
	SDL_Rect rect_;
	const bool operate_;
};

struct texture_clip_rect_setter
{
	// if r is NULL, clip to the full size of the surface.
	texture_clip_rect_setter(const SDL_Rect* r) 
		: original_()
	{
		SDL_Renderer* renderer = get_renderer();
		SDL_RenderGetClipRect(renderer, &original_);
		SDL_RenderSetClipRect(renderer, SDL_RectEmpty(r)? nullptr: r);
	}

	~texture_clip_rect_setter() 
	{
		SDL_RenderSetClipRect(get_renderer(), SDL_RectEmpty(&original_)? nullptr: &original_);
	}

private:
	SDL_Rect original_;
};

class ttexture_alpha_mod_lock
{
public:
	ttexture_alpha_mod_lock(const texture& tex, const uint8_t alpha)
		: tex_(tex)
	{
		SDL_Texture* t = tex_.get();
		SDL_GetTextureAlphaMod(t, &original_modulation_alpha_);
		if (alpha != original_modulation_alpha_) {
			SDL_SetTextureAlphaMod(t, alpha);
		}
	}
	~ttexture_alpha_mod_lock()
	{
		SDL_Texture* t = tex_.get();
		uint8_t modulation_alpha;
		SDL_GetTextureAlphaMod(t, &modulation_alpha);
		if (modulation_alpha != original_modulation_alpha_) {
			SDL_SetTextureAlphaMod(t, original_modulation_alpha_);
		}
	}

private:
	texture tex_;
	uint8_t original_modulation_alpha_;
};

class ttexture_color_mod_lock
{
public:
	ttexture_color_mod_lock(const texture& tex, const uint8_t r, const uint8_t g, const uint8_t b)
		: tex_(tex)
	{
		SDL_Texture* t = tex_.get();
		SDL_GetTextureColorMod(t, &original_modulation_r_, &original_modulation_g_, &original_modulation_b_);
		if (r != original_modulation_r_ || g != original_modulation_g_ || b != original_modulation_b_) {
			SDL_SetTextureColorMod(t, r, g, b);
		}
	}
	~ttexture_color_mod_lock()
	{
		SDL_Texture* t = tex_.get();
		uint8_t modulation_r, modulation_g, modulation_b;
		SDL_GetTextureColorMod(t, &modulation_r, &modulation_g, &modulation_b);
		if (modulation_r != original_modulation_r_ || modulation_g != original_modulation_g_ || modulation_b != original_modulation_b_) {
			SDL_SetTextureColorMod(t, original_modulation_r_, original_modulation_g_, original_modulation_b_);
		}
	}

private:
	texture tex_;
	uint8_t original_modulation_r_;
	uint8_t original_modulation_g_;
	uint8_t original_modulation_b_;
};

#endif
