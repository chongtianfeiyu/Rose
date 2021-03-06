/*
   Copyright (C) 2004 - 2014 by Philippe Plantier <ayin@anathas.org>
   Part of the Battle for Wesnoth Project 

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/**
 * @file
 * Terrain builder.
 */

#include "builder.hpp"

#include "loadscreen.hpp"
#include "map.hpp"
#include "serialization/string_utils.hpp"
#include "image.hpp"
#include "base_map.hpp"

#include <boost/foreach.hpp>
#include "rose_config.hpp"

terrain_builder::building_rule* terrain_builder::building_rules_ = NULL;
uint32_t terrain_builder::building_rules_size_ = 0;
uint32_t terrain_builder::unit_rules_size_;
const std::string terrain_builder::tb_dat_prefix = "tb-";
std::string terrain_builder::using_id;

terrain_builder::tile::tile() :
	flags(),
	images(),
	minimum_unit_index(-1),
	images_foreground(),
	images_background(),
	cached(false)
{}

void terrain_builder::tile::rebuild_cache(const std::string& tod, logs* log)
{
	images_background.clear();
	images_foreground.clear();

	std::vector<rule_image_rand> s_images = images;

	// sort images by their layer (and basey)
	// but use stable to keep the insertion order in equal cases
	std::stable_sort(s_images.begin(), s_images.end());

	BOOST_FOREACH(const rule_image_rand& ri, s_images){
		bool is_background = ri->is_background();

		imagelist& img_list = is_background ? images_background : images_foreground;

		BOOST_FOREACH(const rule_image_variant& variant, ri->variants){
			if(!variant.tods.empty() && variant.tods.find(tod) == variant.tods.end())
				continue;

			//need to break parity pattern in RNG
			/** @todo improve this */
			unsigned int rnd = ri.rand / 7919; //just the 1000th prime
			const animated<image::locator>& anim = variant.images[rnd % variant.images.size()];

			bool is_empty = true;
			for(size_t i = 0; i < anim.get_frames_count(); ++i) {
				if(!image::is_empty_hex(anim.get_frame(i))) {
					is_empty = false;
					break;
				}
			}

			if (is_empty) {
				continue;
			}

			img_list.push_back(anim);
			img_list.back().reset_start_tick();

			if(variant.random_start)
				img_list.back().set_animation_time(ri.rand % img_list.back().get_animation_duration());

			if(log) {
				log->push_back(std::make_pair(&ri, &variant));
			}

			break; // found a matching variant
		}
	}

	// in order to both unit and reduce memroy, clear flags
	flags.clear();
}

void terrain_builder::tile::clear(bool full)
{
	flags.clear();
	if (full) {
		images.clear();
		minimum_unit_index = -1;
	} else if (minimum_unit_index != -1) {
		images.erase(images.begin() + minimum_unit_index, images.end());
		minimum_unit_index = -1;
	} else {
		return;
	}
	// sorted_images = false;
	images_foreground.clear();
	images_background.clear();
	cached = false;
}

static unsigned int get_noise(const map_location& loc, unsigned int index){
	unsigned int a = (loc.x + 92872973) ^ 918273;
	unsigned int b = (loc.y + 1672517) ^ 128123;
	unsigned int c = (index + 127390) ^ 13923787;
	unsigned int abc = a*b*c + a*b + b*c + a*c + a + b + c;
	return abc*abc;
}

void terrain_builder::tilemap::reset(bool full)
{
	for(std::vector<tile>::iterator it = tiles_.begin(); it != tiles_.end(); ++it)
		it->clear(full);
}

void terrain_builder::tilemap::reload(int x, int y)
{
	x_ = x;
	y_ = y;
    std::vector<terrain_builder::tile> new_tiles((x + 4) * (y + 4));
    tiles_.swap(new_tiles);
    reset();
}

bool terrain_builder::tilemap::on_map(const map_location &loc) const
{
	if(loc.x < -2 || loc.y < -2 || loc.x > (x_ + 1) || loc.y > (y_ + 1)) {
		return false;
	}

	return true;

}

terrain_builder::tile& terrain_builder::tilemap::operator[](const map_location &loc)
{
	VALIDATE(on_map(loc), null_str);

	return tiles_[(loc.x + 2) + (loc.y + 2) * (x_ + 4)];
}

const terrain_builder::tile& terrain_builder::tilemap::operator[] (const map_location &loc) const
{
	VALIDATE(on_map(loc), null_str);

	return tiles_[(loc.x + 2) + (loc.y + 2) * (x_ + 4)];
}

extern void wml_building_rules_to_file(const std::string& fname, terrain_builder::building_rule* rules, uint32_t rules_size, uint32_t nfiles, uint32_t sum_size, uint32_t modified);
extern terrain_builder::building_rule* wml_building_rules_from_file(const std::string& fname, uint32_t* rules_size_ptr);

// typical value: 3113
#define MAX_BUILDING_RULES_SIZE		5000

terrain_builder::terrain_builder(const config& cfg, uint32_t nfiles, uint32_t sum_size, uint32_t modified)
	: map_(NULL)
	, tile_map_(0, 0)
	, selector_(SELECTOR_MAP)
	, units_(NULL)
	, terrain_by_type_()
{
	const std::string& id = cfg["id"].str();
	image::terrain_prefix = game_config::terrain::form_img_prefix(id);
	image::precache_file_existence(image::terrain_prefix);
	const std::string offmap_image = "off-map/alpha.png";

	release_heap();
	// resvesed MAX_BUILDING_RULES_SIZE rules, actual rules must not great than it!
	building_rules_ = new terrain_builder::building_rule[MAX_BUILDING_RULES_SIZE];

	//off_map first to prevent some default rule seems to block it
	add_off_map_rule(offmap_image);
	// parse global terrain rules
	parse_global_config(cfg);

	std::stringstream ss;
	ss << game_config::path + "/xwml/" << tb_dat_prefix << id << ".dat";
	wml_building_rules_to_file(ss.str(), building_rules_, building_rules_size_, nfiles, sum_size, modified);
}

terrain_builder::terrain_builder(const std::string& id, const tmap* m) 
	: map_(m)
	, tile_map_(map().w(), map().h())
	, selector_(SELECTOR_MAP)
	, units_(NULL)
	, terrain_by_type_()
{
	if (m && m->tex()) {
		return;
	}

	if (id.empty()) {
		// this is dummy terrain builder.
		return;

	} else if (id != using_id) {
		release_heap();
		using_id = id;
		game_config::terrain::modify_according_tile(id);
		image::switch_tile(id);
	}

	image::precache_file_existence(image::terrain_prefix);

	uint32_t start = SDL_GetTicks();

	if (!building_rules_) {
		std::stringstream ss;
		ss << game_config::path + "/xwml/" << tb_dat_prefix << id << ".dat";
		building_rules_ = wml_building_rules_from_file(ss.str(), &building_rules_size_);
	}

	uint32_t end_to_sdram = SDL_GetTicks();
	SDL_Log("rules to sdram, used time: %u ms\n", end_to_sdram - start);

	// don't support parse local rules
	// parse_config(level);

	if (map_->w() && map_->h()) {
		// don't be called duration dummy builder.
		// when dummy builder, path isn't full, some png maybe don't "exist" when loaded, result to error rule images.
		build_terrains();
	}

	uint32_t end_build_terrains = SDL_GetTicks();
	SDL_Log("build terrains, used time: %u ms\n", end_build_terrains - end_to_sdram);
}

terrain_builder::~terrain_builder()
{
	// release_heap();
}

void terrain_builder::release_heap()
{
	if (building_rules_) {
		delete []building_rules_;
		building_rules_ = NULL;
	}
	building_rules_size_ = 0;
}

void terrain_builder::change_map(const tmap* m)
{
	map_ = m;
	reload_map();
}

const terrain_builder::imagelist *terrain_builder::get_terrain_at(const map_location &loc, const std::string &tod, bool background)
{
	VALIDATE(!map_->tex(), null_str);

	if (!tile_map_.on_map(loc)) {
		return NULL;
	}

	tile& tile_at = tile_map_[loc];

	if (!tile_at.cached) {
		tile_at.rebuild_cache(tod);
		tile_at.cached = true;
	}

	const imagelist& img_list = background? tile_at.images_background : tile_at.images_foreground;

	if (!img_list.empty()) {
		return &img_list;
	}

	return NULL;
}

SDL_Rect terrain_builder::get_terrain_at(const map_location &loc)
{
	if (!tile_map_.on_map(loc)) {
		return empty_rect;
	}

	VALIDATE(map_->tex(), null_str);
	int tile_size = map_->tile_size();

	SDL_Rect clip{loc.x * tile_size, loc.y * tile_size, tile_size, tile_size};
	return clip;
}

bool terrain_builder::update_animation(const map_location &loc)
{
	if(!tile_map_.on_map(loc))
		return false;

	bool changed = false;

	tile& btile = tile_map_[loc];

	BOOST_FOREACH(animated<image::locator>& a, btile.images_background) {
		if(a.need_update())
			changed = true;
		a.update_last_draw_time();
	}
	BOOST_FOREACH(animated<image::locator>& a, btile.images_foreground) {
		if(a.need_update())
			changed = true;
		a.update_last_draw_time();
	}

	return changed;
}

/** @todo TODO: rename this function */
void terrain_builder::rebuild_terrain(const map_location &loc)
{
	if (tile_map_.on_map(loc)) {
		tile& btile = tile_map_[loc];
		// btile.images.clear();
		btile.images_foreground.clear();
		btile.images_background.clear();
		const std::string filename =
			map().get_terrain_info(loc).minimap_image();
		animated<image::locator> img_loc;
		img_loc.add_frame(100,image::locator(image::terrain_prefix + filename + ".png"));
		img_loc.start_animation(0, true);
		btile.images_background.push_back(img_loc);

		//Combine base and overlay image if necessary
		if(map().get_terrain_info(loc).is_combined()) {
			const std::string filename_ovl =
				map().get_terrain_info(loc).minimap_image_overlay();
			animated<image::locator> img_loc_ovl;
			img_loc_ovl.add_frame(100,image::locator(image::terrain_prefix + filename_ovl + ".png"));
			img_loc_ovl.start_animation(0, true);
			btile.images_background.push_back(img_loc_ovl);
		}
	}
}

void terrain_builder::rebuild_terrain()
{
	tile_map_.reset(false);
	terrain_by_type_.clear();
	// in order to reduce memory, reduce terrain_by_type_
	selector_ = SELECTOR_UNIT;
	build_terrains();
	selector_ = SELECTOR_MAP;
}

void terrain_builder::reload_map()
{
	// branch: change map size.
	tile_map_.reload(map_->w(), map_->h());
	terrain_by_type_.clear();
	build_terrains();
}

void terrain_builder::rebuild_all()
{
	// branch: don't change map size. change terrain.
	tile_map_.reset();
	terrain_by_type_.clear();
	build_terrains();
}

static bool image_exists(const std::string& name)
{
	bool precached = name.find("..") == std::string::npos;

	if(precached && image::precached_file_exists(name)) {
		return true;
	} else if(image::exists(name)) {
		return true;
	}

	return false;
}

static std::vector<std::string> get_variations(const std::string& base, const std::string& variations)
{
	/** @todo optimize this function */
	std::vector<std::string> res;
	if(variations.empty()){
		res.push_back(base);
		return res;
	}
	std::string::size_type pos = base.find("@V", 0);
	if(pos == std::string::npos) {
		res.push_back(base);
		return res;
	}
	std::vector<std::string> vars = utils::split(variations, ';', 0);

	BOOST_FOREACH(const std::string& v, vars){
		res.push_back(base);
		std::string::size_type pos = 0;
		while ((pos = res.back().find("@V", pos)) != std::string::npos) {
			res.back().replace(pos, 2, v);
			pos += v.size();
		}
	}
	return res;
}

bool terrain_builder::load_images(building_rule &rule)
{
	rule.image_loaded_ = true;
	// If the rule has no constraints, it is invalid
	if(rule.constraints.empty())
		return false;

	// Parse images and animations data
	// If one is not valid, return false.
	BOOST_FOREACH(terrain_constraint &constraint, rule.constraints)
	{
		BOOST_FOREACH(rule_image& ri, constraint.images)
		{
			BOOST_FOREACH(rule_image_variant& variant, ri.variants)
			{

				std::vector<std::string> var_strings = get_variations(variant.image_string, variant.variations);
				BOOST_FOREACH(const std::string& var, var_strings)
				{
					/** @todo improve this, 99% of terrains are not animated. */
					std::vector<std::string> frames = utils::square_parenthetical_split(var,',');
					animated<image::locator> res;

					BOOST_FOREACH(const std::string& frame, frames)
					{
						const std::vector<std::string> items = utils::split(frame, ':');
						const std::string& str = items.front();

						const size_t tilde = str.find('~');
						bool has_tilde = tilde != std::string::npos;
						const std::string filename = image::terrain_prefix + (has_tilde ? str.substr(0,tilde) : str);

						if (!image_exists(filename)){
							continue; // ignore missing frames
						}

						const std::string modif = (has_tilde ? str.substr(tilde+1) : "");

						int time = 100;
						if(items.size() > 1) {
							time = atoi(items.back().c_str());
						}
						image::locator locator;
						if(ri.global_image) {
							locator = image::locator(filename, constraint.loc, ri.center_x, ri.center_y, modif);
						} else {
							locator = image::locator(filename, modif);
						}
						res.add_frame(time, locator);
					}
					if(res.get_frames_count() == 0)
						break; // no valid images, don't register it

					res.start_animation(0, true);
					variant.images.push_back(res);
				}
				if(variant.images.empty())
					return false; //no valid images, rule is invalid
			}
		}
	}

	return true;
}

void terrain_builder::rotate(terrain_constraint &ret, int angle)
{
	static const struct { int ii; int ij; int ji; int jj; }  rotations[6] =
		{ {  1, 0, 0,  1 }, {  1,  1, -1, 0 }, { 0,  1, -1, -1 },
		  { -1, 0, 0, -1 }, { -1, -1,  1, 0 }, { 0, -1,  1,  1 } };

	// The following array of matrices is intended to rotate the (x,y)
	// coordinates of a point in a wesnoth hex (and wesnoth hexes are not
	// regular hexes :) ).
	// The base matrix for a 1-step rotation with the wesnoth tile shape
	// is:
	//
	// r = s^-1 * t * s
	//
	// with s = [[ 1   0         ]
	//           [ 0   -sqrt(3)/2 ]]
	//
	// and t =  [[ -1/2       sqrt(3)/2 ]
	//           [ -sqrt(3)/2  1/2        ]]
	//
	// With t being the rotation matrix (pi/3 rotation), and s a matrix
	// that transforms the coordinates of the wesnoth hex to make them
	// those of a regular hex.
	//
	// (demonstration left as an exercise for the reader)
	//
	// So we have
	//
	// r = [[ 1/2  -3/4 ]
	//      [ 1    1/2  ]]
	//
	// And the following array contains I(2), r, r^2, r^3, r^4, r^5
	// (with r^3 == -I(2)), which are the successive rotations.
	static const struct {
		double xx;
		double xy;
		double yx;
		double yy;
	} xyrotations[6] = {
		{ 1.,         0.,  0., 1.    },
		{ 1./2. , -3./4.,  1., 1./2. },
		{ -1./2., -3./4.,   1, -1./2.},
		{ -1.   ,     0.,  0., -1.   },
		{ -1./2.,  3./4., -1., -1./2.},
		{ 1./2. ,  3./4., -1., 1./2. },
	};

	VALIDATE(angle >= 0, null_str);

	angle %= 6;

	// Vector i is going from n to s, vector j is going from ne to sw.
	int vi = ret.loc.y - ret.loc.x/2;
	int vj = ret.loc.x;

	int ri = rotations[angle].ii * vi + rotations[angle].ij * vj;
	int rj = rotations[angle].ji * vi + rotations[angle].jj * vj;

	ret.loc.x = rj;
	ret.loc.y = ri + (rj >= 0 ? rj/2 : (rj-1)/2);

	for (rule_imagelist::iterator itor = ret.images.begin();
			itor != ret.images.end(); ++itor) {

		double vx, vy, rx, ry;

		vx = double(itor->basex) - double(TILEWIDTH)/2;
		vy = double(itor->basey) - double(TILEWIDTH)/2;

		rx = xyrotations[angle].xx * vx + xyrotations[angle].xy * vy;
		ry = xyrotations[angle].yx * vx + xyrotations[angle].yy * vy;

		itor->basex = int(rx + TILEWIDTH/2);
		itor->basey = int(ry + TILEWIDTH/2);

		//std::cerr << "Rotation: from " << vx << ", " << vy << " to " << itor->basex <<
		//	", " << itor->basey << "\n";
	}
}

void terrain_builder::replace_rotate_tokens(std::string &s, int angle,
	const std::vector<std::string> &replacement)
{
	std::string::size_type pos = 0;
	while ((pos = s.find("@R", pos)) != std::string::npos) {
		if (pos + 2 >= s.size()) return;
		unsigned i = s[pos + 2] - '0' + angle;
		if (i >= 6) i -= 6;
		if (i >= 6) { pos += 2; continue; }
		const std::string &r = replacement[i];
		s.replace(pos, 3, r);
		pos += r.size();
	}
}

void terrain_builder::replace_rotate_tokens(rule_image &image, int angle,
	const std::vector<std::string> &replacement)
{
	BOOST_FOREACH(rule_image_variant& variant, image.variants) {
		replace_rotate_tokens(variant, angle, replacement);
	}
}

void terrain_builder::replace_rotate_tokens(rule_imagelist &list, int angle,
	const std::vector<std::string> &replacement)
{
	BOOST_FOREACH(rule_image &img, list) {
		replace_rotate_tokens(img, angle, replacement);
	}
}

void terrain_builder::replace_rotate_tokens(building_rule &rule, int angle,
	const std::vector<std::string> &replacement)
{
	BOOST_FOREACH(terrain_constraint &cons, rule.constraints)
	{
		// Transforms attributes
		BOOST_FOREACH(std::string &flag, cons.set_flag) {
			replace_rotate_tokens(flag, angle, replacement);
		}
		BOOST_FOREACH(std::string &flag, cons.no_flag) {
			replace_rotate_tokens(flag, angle, replacement);
		}
		BOOST_FOREACH(std::string &flag, cons.has_flag) {
			replace_rotate_tokens(flag, angle, replacement);
		}
		replace_rotate_tokens(cons.images, angle, replacement);
	}

	//replace_rotate_tokens(rule.images, angle, replacement);
}

void terrain_builder::rotate_rule(building_rule &ret, int angle,
	const std::vector<std::string> &rot)
{
	if (rot.size() != 6) {
		VALIDATE(false, "invalid rotations");
		return;
	}

	BOOST_FOREACH(terrain_constraint &cons, ret.constraints) {
		rotate(cons, angle);
	}

	// Normalize the rotation, so that it starts on a positive location
	int minx = INT_MAX;
	int miny = INT_MAX;

	BOOST_FOREACH(const terrain_constraint &cons, ret.constraints) {
		minx = std::min<int>(cons.loc.x, minx);
		miny = std::min<int>(2 * cons.loc.y + (cons.loc.x & 1), miny);
	}

	if((miny & 1) && (minx & 1) && (minx < 0))
		miny += 2;
	if(!(miny & 1) && (minx & 1) && (minx > 0))
		miny -= 2;

	BOOST_FOREACH(terrain_constraint &cons, ret.constraints) {
		cons.loc.legacy_sum_assign(map_location(-minx, -((miny - 1) / 2)));
	}

	replace_rotate_tokens(ret, angle, rot);
}

terrain_builder::rule_image_variant::rule_image_variant(const std::string &image_string, const std::string& variations, const std::string& tod, bool random_start) :
		image_string(image_string),
		variations(variations),
		images(),
		tods(),
		random_start(random_start)
{
	if(!tod.empty()) {
		const std::vector<std::string> tod_list = utils::split(tod);
		tods.insert(tod_list.begin(), tod_list.end());
	}
}

void terrain_builder::add_images_from_config(rule_imagelist& images, const config &cfg, bool global, int dx, int dy)
{
	BOOST_FOREACH(const config &img, cfg.child_range("image"))
	{
		int layer = img["layer"];

		int basex = TILEWIDTH / 2 + dx, basey = TILEWIDTH / 2 + dy;
		if (const config::attribute_value *base_ = img.get("base")) {
			std::vector<std::string> base = utils::split(*base_);
			if(base.size() >= 2) {
				basex = atoi(base[0].c_str());
				basey = atoi(base[1].c_str());
			}
		}

		int center_x = -1, center_y = -1;
		if (const config::attribute_value *center_ = img.get("center")) {
			std::vector<std::string> center = utils::split(*center_);
			if(center.size() >= 2) {
				center_x = atoi(center[0].c_str());
				center_y = atoi(center[1].c_str());
			}
		}

		images.push_back(rule_image(layer, basex - dx, basey - dy, global, center_x, center_y));

		// Adds the other variants of the image
		BOOST_FOREACH(const config &variant, img.child_range("variant"))
		{
			const std::string &name = variant["name"];
			const std::string &variations = img["variations"];
			const std::string &tod = variant["tod"];
			bool random_start = variant["random_start"].to_bool(true);

			images.back().variants.push_back(rule_image_variant(name, variations, tod, random_start));
		}

		// Adds the main (default) variant of the image at the end,
		// (will be used only if previous variants don't match)
		const std::string &name = img["name"];
		const std::string &variations = img["variations"];
		bool random_start = img["random_start"].to_bool(true);
		images.back().variants.push_back(rule_image_variant(name, variations, random_start));
	}
}

terrain_builder::terrain_constraint &terrain_builder::add_constraints(
		terrain_builder::constraint_set& constraints,
		const map_location& loc,
		const t_translation::t_match& type, const config& global_images)
{
	terrain_constraint *cons = NULL;
	BOOST_FOREACH(terrain_constraint &c, constraints) {
		if (c.loc == loc) {
			cons = &c;
			break;
		}
	}

	if (!cons) {
		// The terrain at the current location did not exist, so create it
		constraints.push_back(terrain_constraint(loc));
		cons = &constraints.back();
	}

	if(!type.terrain.empty()) {
		cons->terrain_types_match = type;
	}

	int x = loc.x * TILEWIDTH * 3 / 4;
	int y = loc.y * TILEWIDTH + (loc.x % 2) * TILEWIDTH / 2;
	add_images_from_config(cons->images, global_images, true, x, y);

	return *cons;
}

void terrain_builder::add_constraints(terrain_builder::constraint_set &constraints,
		const map_location& loc, const config& cfg, const config& global_images)

{
	terrain_constraint& constraint = add_constraints(constraints, loc,
		t_translation::t_match(cfg["type"], t_translation::WILDCARD), global_images);


	std::vector<std::string> item_string = utils::square_parenthetical_split(cfg["set_flag"],',',"[","]");
	constraint.set_flag.insert(constraint.set_flag.end(),
			item_string.begin(), item_string.end());

	item_string = utils::square_parenthetical_split(cfg["has_flag"],',',"[","]");
	constraint.has_flag.insert(constraint.has_flag.end(),
			item_string.begin(), item_string.end());

	item_string = utils::square_parenthetical_split(cfg["no_flag"],',',"[","]");
	constraint.no_flag.insert(constraint.no_flag.end(),
			item_string.begin(), item_string.end());

	item_string = utils::square_parenthetical_split(cfg["set_no_flag"],',',"[","]");
	constraint.set_flag.insert(constraint.set_flag.end(),
			item_string.begin(), item_string.end());
	constraint.no_flag.insert(constraint.no_flag.end(),
			item_string.begin(), item_string.end());


	add_images_from_config(constraint.images, cfg, false);
}

void terrain_builder::parse_mapstring(const std::string &mapstring,
		struct building_rule &br, anchormap& anchors,
		const config& global_images)
{

	const t_translation::t_map map = t_translation::read_builder_map(mapstring);

	// If there is an empty map leave directly.
	// Determine after conversion, since a
	// non-empty string can return an empty map.
	if(map.empty()) {
		return;
	}

	int lineno = (map[0][0] == t_translation::NONE_TERRAIN) ? 1 : 0;
	int x = lineno;
	int y = 0;
	for(size_t y_off = 0; y_off < map.size(); ++y_off) {
		for(size_t x_off = x; x_off < map[y_off].size(); ++x_off) {

			const t_translation::t_terrain terrain = map[y_off][x_off];

			if(terrain.base == t_translation::TB_DOT) {
				// Dots are simple placeholders,
				// which do not represent actual terrains.
			} else if (terrain.overlay != 0 ) {
				anchors.insert(std::pair<int, map_location>(terrain.overlay, map_location(x, y)));
			} else if (terrain.base == t_translation::TB_STAR) {
				add_constraints(br.constraints, map_location(x, y), t_translation::STAR, global_images);
			} else {
				std::stringstream err;
				err << "Invalid terrain (" << t_translation::write_terrain_code(terrain) << ") in builder map";
				VALIDATE(false, err.str());
				return;
			}
		x += 2;
		}

		if(lineno % 2 == 1) {
			++y;
			x = 0;
		} else {
			x = 1;
		}
		++lineno;
	}
}

void terrain_builder::add_rule(building_rule* rules, building_rule &rule)
{
	if (load_images(rule)) {
		rules[building_rules_size_ ++] = rule;
	}
}

void terrain_builder::add_rotated_rules(building_rule* rules, building_rule &tpl,
	const std::string &rotations)
{
	if(rotations.empty()) {
		// Adds the parsed built terrain to the list

		add_rule(rules, tpl);
	} else {
		const std::vector<std::string>& rot = utils::split(rotations, ',');

		for(size_t angle = 0; angle < rot.size(); ++angle) {
			/* Only 5% of the rules have valid images, so most of
			   them will be discarded. If the ratio was higher,
			   it would be more efficient to insert a copy of the
			   template rule into the ruleset, modify it in place,
			   and remove it if invalid. But since the ratio is so
			   low, the speedup is not worth the extra multiset
			   manipulations. */
			building_rule rule = tpl;
			rotate_rule(rule, angle, rot);
			add_rule(rules, rule);
		}
	}
}

void terrain_builder::parse_config(const config &cfg, bool local)
{
	// Parses the list of building rules (BRs)
	BOOST_FOREACH(const config &br, cfg.child_range("terrain_graphics"))
	{
		building_rule pbr; // Parsed Building rule
		pbr.local = local;

		// add_images_from_config(pbr.images, **br);

		pbr.location_constraints =
			map_location(br["x"].to_int() - 1, br["y"].to_int() - 1);

		pbr.probability = br["probability"].to_int(100);

		// Mapping anchor indices to anchor locations.
		anchormap anchors;

		// Parse the map= , if there is one (and fill the anchors list)
		parse_mapstring(br["map"], pbr, anchors, br);

		// Parses the terrain constraints (TCs)
		BOOST_FOREACH(const config &tc, br.child_range("tile"))
		{
			// Adds the terrain constraint to the current built terrain's list
			// of terrain constraints, if it does not exist.
			map_location loc;
			if (const config::attribute_value *v = tc.get("x")) {
				loc.x = *v;
			}
			if (const config::attribute_value *v = tc.get("y")) {
				loc.y = *v;
			}
			if (const config::attribute_value *v = tc.get("loc")) {
				std::vector<std::string> sloc = utils::split(*v);
				if(sloc.size() == 2) {
					loc.x = atoi(sloc[0].c_str());
					loc.y = atoi(sloc[1].c_str());
				}
			}
			if(loc.valid()) {
				add_constraints(pbr.constraints, loc, tc, br);
			}
			if (const config::attribute_value *v = tc.get("pos")) {
				int pos = *v;
				if(anchors.find(pos) == anchors.end()) {
					// Invalid anchor!
					continue;
				}

				std::pair<anchormap::const_iterator, anchormap::const_iterator> range =
					anchors.equal_range(pos);

				for(; range.first != range.second; ++range.first) {
					loc = range.first->second;
					add_constraints(pbr.constraints, loc, tc, br);
				}
			}
		}

		const std::vector<std::string> global_set_flag = utils::split(br["set_flag"]);
		const std::vector<std::string> global_no_flag = utils::split(br["no_flag"]);
		const std::vector<std::string> global_has_flag = utils::split(br["has_flag"]);
		const std::vector<std::string> global_set_no_flag = utils::split(br["set_no_flag"]);

		BOOST_FOREACH(terrain_constraint &constraint, pbr.constraints)
		{
			constraint.set_flag.insert(constraint.set_flag.end(),
				global_set_flag.begin(), global_set_flag.end());
			constraint.no_flag.insert(constraint.no_flag.end(),
				global_no_flag.begin(), global_no_flag.end());
			constraint.has_flag.insert(constraint.has_flag.end(),
				global_has_flag.begin(), global_has_flag.end());
			constraint.set_flag.insert(constraint.set_flag.end(),
				global_set_no_flag.begin(), global_set_no_flag.end());
			constraint.no_flag.insert(constraint.no_flag.end(),
				global_set_no_flag.begin(), global_set_no_flag.end());
		}

		// Handles rotations
		const std::string &rotations = br["rotations"];

		pbr.precedence = br["precedence"];

		add_rotated_rules(building_rules_, pbr, rotations);

		loadscreen::increment_progress();
	}

// Debug output for the terrain rules
#if 0
	std::cerr << "Built terrain rules: \n";

	building_ruleset::const_iterator rule;
	for(rule = building_rules_.begin(); rule != building_rules_.end(); ++rule) {
		std::cerr << ">> New rule: image_background = "
			<< "\n>> Location " << rule->second.location_constraints
			<< "\n>> Probability " << rule->second.probability

		for(constraint_set::const_iterator constraint = rule->second.constraints.begin();
		    constraint != rule->second.constraints.end(); ++constraint) {

			std::cerr << ">>>> New constraint: location = (" << constraint->second.loc
			          << "), terrain types = '" << t_translation::write_list(constraint->second.terrain_types_match.terrain) << "'\n";

			std::vector<std::string>::const_iterator flag;

			for(flag  = constraint->second.set_flag.begin(); flag != constraint->second.set_flag.end(); ++flag) {
				std::cerr << ">>>>>> Set_flag: " << *flag << "\n";
			}

			for(flag = constraint->second.no_flag.begin(); flag != constraint->second.no_flag.end(); ++flag) {
				std::cerr << ">>>>>> No_flag: " << *flag << "\n";
			}
		}

	}
#endif

}

void terrain_builder::add_off_map_rule(const std::string& image)
{
	// Build a config object
	config cfg;

	config &item = cfg.add_child("terrain_graphics");

	config &tile = item.add_child("tile");
	tile["x"] = 0;
	tile["y"] = 0;
	tile["type"] = t_translation::write_terrain_code(t_translation::OFF_MAP_USER);

	config &tile_image = tile.add_child("image");
	tile_image["layer"] = -1000;
	tile_image["name"] = image;

	item["probability"] = 100;
	item["no_flag"] = "base";
	item["set_flag"] = "base";

	// Parse the object
	parse_global_config(cfg);
}

bool terrain_builder::rule_matches(const terrain_builder::building_rule &rule,
		const map_location &loc, const terrain_constraint *type_checked) const
{
	if(rule.location_constraints.valid() && rule.location_constraints != loc) {
		return false;
	}

	if(rule.probability != 100) {
		unsigned int random = get_noise(loc, rule.get_hash()) % 100;
		if(random > static_cast<unsigned int>(rule.probability)) {
			return false;
		}
	}

	BOOST_FOREACH(const terrain_constraint &cons, rule.constraints)
	{
		// Translated location
		const map_location tloc = loc.legacy_sum(cons.loc);

		if(!tile_map_.on_map(tloc)) {
			return false;
		}

		//std::cout << "testing..." << builder_letter(map().get_terrain(tloc))

		// check if terrain matches except if we already know that it does
		if (&cons != type_checked) {
			if (selector_ == SELECTOR_MAP) {
				if (!terrain_matches(map().get_terrain(tloc), cons.terrain_types_match)) {
					return false;
				}
			} else if (!units_->terrain_matches(tloc, cons.terrain_types_match)) {
				return false;
			}
		}
		const std::set<std::string> &flags = tile_map_[tloc].flags;

		BOOST_FOREACH(const std::string &s, cons.no_flag) {
			// If a flag listed in "no_flag" is present, the rule does not match
			if (flags.find(s) != flags.end()) {
				return false;
			}
		}
		BOOST_FOREACH(const std::string &s, cons.has_flag) {
			// If a flag listed in "has_flag" is not present, this rule does not match
			if (flags.find(s) == flags.end()) {
				return false;
			}
		}
	}

	return true;
}

void terrain_builder::apply_rule(const terrain_builder::building_rule &rule, const map_location &loc)
{
	unsigned int rand_seed = get_noise(loc, rule.get_hash());

	BOOST_FOREACH(const terrain_constraint &constraint, rule.constraints)
	{
		const map_location tloc = loc.legacy_sum(constraint.loc);
		if(!tile_map_.on_map(tloc)) {
			return;
		}

		tile& btile = tile_map_[tloc];

		BOOST_FOREACH(const rule_image &img, constraint.images) {
			if (selector_ == SELECTOR_UNIT && btile.minimum_unit_index == -1) {
				btile.minimum_unit_index = btile.images.size();
				// set need cached flag.	
				btile.cached = false;
			}
			btile.images.push_back(tile::rule_image_rand(&img, rand_seed));
		}

		// Sets flags
		BOOST_FOREACH(const std::string &flag, constraint.set_flag) {
			btile.flags.insert(flag);
		}

	}
}

// copied from text_surface::hash()
// but keep it separated because the needs are different
// and changing it will modify the map random variations
static unsigned int hash_str(const std::string& str)
{
	unsigned int h = 0;
	for(std::string::const_iterator it = str.begin(), it_end = str.end(); it != it_end; ++it)
		h = ((h << 9) | (h >> (sizeof(int) * 8 - 9))) ^ (*it);
	return h;
}

unsigned int terrain_builder::building_rule::get_hash() const
{
	if(hash_ != DUMMY_HASH)
		return hash_;

	BOOST_FOREACH(const terrain_constraint &constraint, constraints) {
		BOOST_FOREACH(const rule_image& ri, constraint.images) {
			BOOST_FOREACH(const rule_image_variant& variant, ri.variants) {
				// we will often hash the same string, but that seems fast enough
				hash_ += hash_str(variant.image_string);
			}
		}
	}

	//don't use the reserved dummy hash
	if(hash_ == DUMMY_HASH)
		hash_ = 105533;  // just a random big prime number

	return hash_;
}

void terrain_builder::build_terrains()
{
	// Builds the terrain_by_type_ cache
	if (selector_ == SELECTOR_MAP) {
		for(int x = -2; x <= map().w(); ++x) {
			for(int y = -2; y <= map().h(); ++y) {
				const map_location loc(x,y);
				const t_translation::t_terrain t = map().get_terrain(loc);

				terrain_by_type_[t].push_back(loc);
			}
		}
	} else {
		units_->build_terrains(terrain_by_type_);
	}

	uint32_t min_rule, max_rule;
	if (building_rules_size_ == 0) {
		min_rule = max_rule = 0;
	} else if (selector_ == SELECTOR_MAP) {
		min_rule = 0;
		max_rule = building_rules_size_ - unit_rules_size_;
	} else {
		min_rule = building_rules_size_ - unit_rules_size_;
		max_rule = building_rules_size_;
	}
	for (uint32_t rule_index = min_rule; rule_index < max_rule; rule_index ++) {
		building_rule& rule = building_rules_[rule_index];
		// Find the constraint that contains the less terrain of all terrain rules.
		// We will keep a track of the matching terrains of this constraint
		// and later try to apply the rule only on them
		size_t min_size = INT_MAX;
		t_translation::t_list min_types;
		const terrain_constraint *min_constraint = NULL;

		BOOST_FOREACH(const terrain_constraint &constraint, rule.constraints)
		{
			const t_translation::t_match& match = constraint.terrain_types_match;
			t_translation::t_list matching_types;
			size_t constraint_size = 0;

			for (terrain_by_type_map::iterator type_it = terrain_by_type_.begin();
					 type_it != terrain_by_type_.end(); ++type_it) {

				const t_translation::t_terrain t = type_it->first;
				if (terrain_matches(t, match)) {
					const size_t match_size = type_it->second.size();
					constraint_size += match_size;
					if (constraint_size >= min_size) {
						break; // not a minimum, bail out
					}
					matching_types.push_back(t);
				}
			}

			// if (constraint_size < min_size) {
			if ((selector_ == SELECTOR_MAP || constraint_size) && constraint_size < min_size) {
				min_size = constraint_size;
				min_types = matching_types;
				min_constraint = &constraint;
				if (min_size == 0) {
				 	// a constraint is never matched on this map
				 	// we break with a empty type list
					break;
				}
			}
		}

		//NOTE: if min_types is not empty, we have found a valid min_constraint;
		for(t_translation::t_list::const_iterator t = min_types.begin();
				t != min_types.end(); ++t) {

			const std::vector<map_location>* locations = &terrain_by_type_[*t];

			for(std::vector<map_location>::const_iterator itor = locations->begin();
					itor != locations->end(); ++itor) {
				const map_location loc = itor->legacy_difference(min_constraint->loc);

				if(rule_matches(rule, loc, min_constraint)) {
					if (!rule.image_loaded_) {
						load_images(rule);
					}
					apply_rule(rule, loc);
				}
			}
		}

	}

	// in order to reduce memory, release terrain_by_type_
	// but in map_type of siege, require this variable.
	// retain it when total grid less than 400.
	if (map_->w() * map_->h() > 400) {
		terrain_by_type_.clear();
	}
}

terrain_builder::tile* terrain_builder::get_tile(const map_location &loc)
{
	if(tile_map_.on_map(loc))
		return &(tile_map_[loc]);
	return NULL;
}
