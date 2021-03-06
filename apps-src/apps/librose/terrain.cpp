/* $Id: terrain.cpp 55983 2013-01-01 09:22:03Z mordante $ */
/*
   Copyright (C) 2003 - 2013 by David White <dave@whitevine.net>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "global.hpp"

#include "gettext.hpp"
#include "serialization/string_utils.hpp"
#include "terrain.hpp"
#include "util.hpp"

#include <boost/foreach.hpp>

#include <set>

terrain_type::terrain_type() :
		minimap_image_(),
		minimap_image_overlay_(),
		editor_image_(),
		id_(),
		name_(),
		editor_name_(),
		description_(),
		number_(t_translation::VOID_TERRAIN),
		mvt_type_(1, t_translation::VOID_TERRAIN),
		def_type_(1, t_translation::VOID_TERRAIN),
		union_type_(1, t_translation::VOID_TERRAIN),
		height_adjust_(0),
		height_adjust_set_(false),
		submerge_(0.0),
		submerge_set_(false),
		light_modification_(0),
		max_light_(0),
		min_light_(0),
		heals_(0),
		income_description_(),
		income_description_ally_(),
		income_description_enemy_(),
		income_description_own_(),
		editor_group_(),
		village_(false),
		castle_(false),
		keep_(false),
		overlay_(false),
		combined_(false),
		editor_default_base_(t_translation::VOID_TERRAIN),
		hide_in_editor_(false)
{}

terrain_type::terrain_type(const config& cfg) :
		minimap_image_(cfg["symbol_image"]),
		minimap_image_overlay_(),
		editor_image_(cfg["editor_image"]),
		id_(cfg["id"]),
		name_(cfg["name"].t_str()),
		editor_name_(cfg["editor_name"].t_str()),
		description_(cfg["description"].t_str()),
		number_(t_translation::read_terrain_code(cfg["string"])),
		mvt_type_(),
		def_type_(),
		union_type_(),
		aliasof_(),
		height_adjust_(cfg["unit_height_adjust"]),
		height_adjust_set_(!cfg["unit_height_adjust"].empty()),
		submerge_(cfg["submerge"].to_double()),
		submerge_set_(!cfg["submerge"].empty()),
		light_modification_(cfg["light"]),
		max_light_(cfg["max_light"].to_int(light_modification_)),
		min_light_(cfg["min_light"].to_int(light_modification_)),
		heals_(cfg["heals"]),
		income_description_(),
		income_description_ally_(),
		income_description_enemy_(),
		income_description_own_(),
		editor_group_(cfg["editor_group"]),
		village_(cfg["gives_income"].to_bool()),
		castle_(cfg["recruit_onto"].to_bool()),
		keep_(cfg["recruit_from"].to_bool()),
		overlay_(number_.base == t_translation::NO_LAYER),
		combined_(false),
		editor_default_base_(t_translation::read_terrain_code(cfg["default_base"])),
		hide_in_editor_(cfg["hidden"].to_bool(false))
{
/**
 *  @todo reenable these validations. The problem is that all MP
 *  scenarios/campaigns share the same namespace and one rogue scenario
 *  can avoid the player to create a MP game. So every scenario/campaign
 *  should get its own namespace to be safe.
 */
#if 0
	VALIDATE(number_ != t_translation::NONE_TERRAIN,
		missing_mandatory_wml_key("terrain_type", "string"));
	VALIDATE(!minimap_image_.empty(),
		missing_mandatory_wml_key("terrain_type", "symbol_image", "string",
		t_translation::write_terrain_code(number_)));
	VALIDATE(!name_.empty(),
		missing_mandatory_wml_key("terrain_type", "name", "string",
		t_translation::write_terrain_code(number_)));
#endif

	if(editor_image_.empty()) {
		editor_image_ = minimap_image_;
	}

	mvt_type_.push_back(number_);
	def_type_.push_back(number_);
	aliasof_ = cfg["aliasof"].str();
	const t_translation::t_list& alias = t_translation::read_list(aliasof_);
	if(!alias.empty()) {
		mvt_type_ = alias;
		def_type_ = alias;
	}

	const t_translation::t_list& mvt_alias = t_translation::read_list(cfg["mvt_alias"]);
	if(!mvt_alias.empty()) {
		mvt_type_ = mvt_alias;
	}

	const t_translation::t_list& def_alias = t_translation::read_list(cfg["def_alias"]);
	if(!def_alias.empty()) {
		def_type_ = def_alias;
	}
	union_type_ = mvt_type_;
	union_type_.insert( union_type_.end(), def_type_.begin(), def_type_.end() );

	// remove + and -
	union_type_.erase(std::remove(union_type_.begin(), union_type_.end(),
				t_translation::MINUS), union_type_.end());

	union_type_.erase(std::remove(union_type_.begin(), union_type_.end(),
				t_translation::PLUS), union_type_.end());

	// remove doubles
	std::sort(union_type_.begin(),union_type_.end());
	union_type_.erase(std::unique(union_type_.begin(), union_type_.end()), union_type_.end());



	//mouse over message are only shown on villages
	if(village_) {
		income_description_ = cfg["income_description"];
		if(income_description_ == "") {
			income_description_ = _("Village");
		}

		income_description_ally_ = cfg["income_description_ally"];
		if(income_description_ally_ == "") {
			income_description_ally_ = _("Allied village");
		}

		income_description_enemy_ = cfg["income_description_enemy"];
		if(income_description_enemy_ == "") {
			income_description_enemy_ = _("Enemy village");
		}

		income_description_own_ = cfg["income_description_own"];
		if(income_description_own_ == "") {
			income_description_own_ = _("Owned village");
		}
	}
}

terrain_type::terrain_type(const terrain_type& base, const terrain_type& overlay) :
	minimap_image_(base.minimap_image_),
	minimap_image_overlay_(overlay.minimap_image_),
	editor_image_(overlay.editor_image_),
	id_(base.id_+"^"+overlay.id_),
	name_(overlay.name_),
	editor_name_(overlay.editor_name_),
	description_(overlay.description()),
	number_(t_translation::t_terrain(base.number_.base, overlay.number_.overlay)),
	mvt_type_(overlay.mvt_type_),
	def_type_(overlay.def_type_),
	union_type_(),
	aliasof_(),
	height_adjust_(base.height_adjust_),
	height_adjust_set_(base.height_adjust_set_),
	submerge_(base.submerge_),
	submerge_set_(base.submerge_set_),
	light_modification_(base.light_modification_ + overlay.light_modification_),
	max_light_(std::max(base.max_light_, overlay.max_light_)),
	min_light_(std::min(base.min_light_, overlay.min_light_)),
	heals_(std::max<int>(base.heals_, overlay.heals_)),
	income_description_(),
	income_description_ally_(),
	income_description_enemy_(),
	income_description_own_(),
	editor_group_(),
	village_(base.village_ || overlay.village_),
	castle_(base.castle_ || overlay.castle_),
	keep_(base.keep_ || overlay.keep_),
	overlay_(false),
	combined_(true),
	editor_default_base_(),
	hide_in_editor_(base.hide_in_editor_ || overlay.hide_in_editor_)
{
	if(description_.empty()) {
		description_ = base.description();
	}

	if(overlay.height_adjust_set_) {
		height_adjust_set_ = true;
		height_adjust_ = overlay.height_adjust_;
	}

	if(overlay.submerge_set_) {
		submerge_set_ = true;
		submerge_ = overlay.submerge_;
	}

	merge_alias_lists(mvt_type_, base.mvt_type_);
	merge_alias_lists(def_type_, base.def_type_);

	union_type_ = mvt_type_;
	union_type_.insert( union_type_.end(), def_type_.begin(), def_type_.end() );

	// remove + and -
	union_type_.erase(std::remove(union_type_.begin(), union_type_.end(),
				t_translation::MINUS), union_type_.end());

	union_type_.erase(std::remove(union_type_.begin(), union_type_.end(),
				t_translation::PLUS), union_type_.end());

	// remove doubles
	std::sort(union_type_.begin(),union_type_.end());
	union_type_.erase(std::unique(union_type_.begin(), union_type_.end()), union_type_.end());



	//mouse over message are only shown on villages
	if(base.village_) {
		income_description_ = base.income_description_;
		income_description_ally_ = base.income_description_ally_;
		income_description_enemy_ = base.income_description_enemy_;
		income_description_own_ = base.income_description_own_;
	}
	else if (overlay.village_) {
		income_description_ = overlay.income_description_;
		income_description_ally_ = overlay.income_description_ally_;
		income_description_enemy_ = overlay.income_description_enemy_;
		income_description_own_ = overlay.income_description_own_;
	}

}

t_translation::t_terrain terrain_type::terrain_with_default_base() const {
	if(overlay_ && editor_default_base_ != t_translation::NONE_TERRAIN) {
		return t_translation::t_terrain(editor_default_base_.base, number_.overlay);
	}
	return number_;
}

bool terrain_type::operator==(const terrain_type& other) const {
	return minimap_image_         == other.minimap_image_
		&& minimap_image_overlay_ == other.minimap_image_overlay_
		&& editor_image_          == other.editor_image_
		&& id_                    == other.id_
		&& name_.base_str()       == other.name_.base_str()
		&& editor_name_.base_str() == other.editor_name_.base_str()
		&& number_                == other.number_
		&& height_adjust_         == other.height_adjust_
		&& height_adjust_set_     == other.height_adjust_set_
		&& submerge_              == other.submerge_
		&& submerge_set_          == other.submerge_set_
		&& light_modification_    == other.light_modification_
		&& max_light_             == other.max_light_
		&& min_light_             == other.min_light_
		&& heals_                 == other.heals_
		&& village_               == other.village_
		&& castle_                == other.castle_
		&& keep_                  == other.keep_
		&& editor_default_base_   == other.editor_default_base_
		&& hide_in_editor_        == other.hide_in_editor_;
}

void create_terrain_maps(const config::const_child_itors &cfgs,
                         t_translation::t_list& terrain_list,
                         std::map<t_translation::t_terrain, terrain_type>& letter_to_terrain)
{
	BOOST_FOREACH(const config &t, cfgs)
	{
		terrain_type terrain(t);
		// create_terrain_maps: $terrain.number() $terrain.id() $terrain.name() : $terrain.editor_group().

		std::pair<std::map<t_translation::t_terrain, terrain_type>::iterator, bool> res;
		res = letter_to_terrain.insert(std::make_pair(terrain.number(), terrain));
		if (!res.second) {
			terrain_type& curr = res.first->second;
			if(terrain == curr) {
				// Merging terrain $terrain.number(): $terrain.id() ($terrain.name())
				std::vector<std::string> eg1 = utils::split(curr.editor_group());
				std::vector<std::string> eg2 = utils::split(terrain.editor_group());
				std::set<std::string> egs;
				bool clean_merge = true;
				BOOST_FOREACH(std::string& t, eg1)
					clean_merge &= egs.insert(t).second;
				BOOST_FOREACH(std::string& t, eg2)
					clean_merge &= egs.insert(t).second;

				std::string joined = utils::join(egs);
				curr.set_editor_group(joined);
				if(clean_merge) {
					// Editor groups merged to: $joined
				} else {
					// Merged terrain $terrain.number(): $terrain.id() ($terrain.name()) 
					// with duplicate editor groups [$terrain.editor_group()] and [$curr.editor_group()]
				}
			} else {
				// Duplicate terrain code definition found for $terrain.number()
				// Failed to add terrain $terrain.id()("$terrain.name()") [$terrain.editor_group()]
				// which conflicts with $curr.id() ($curr.name()) [$curr.editor_group()]
			}
		} else {
			terrain_list.push_back(terrain.number());
		}
	}
}

void merge_alias_lists(t_translation::t_list& first, const t_translation::t_list& second)
{
	// Insert second vector into first when the terrain _ref^base is encountered

	bool revert = (first.front() == t_translation::MINUS ? true : false);
	t_translation::t_list::iterator i;

	for(i = first.begin(); i != first.end(); ++i) {
		if(*i == t_translation::PLUS) {
			revert = false;
			continue;
		} else if(*i == t_translation::MINUS) {
			revert = true;
			continue;
		}

		if(*i == t_translation::BASE) {
			t_translation::t_list::iterator insert_it = first.erase(i);
			//if we are in reverse mode, insert PLUS before and MINUS after the base list
            //so calculation of base aliases will work normal
			if(revert) {
//				insert_it = first.insert(insert_it, t_translation::PLUS);
//				insert_it++;
				insert_it = first.insert(insert_it, t_translation::MINUS);
			}
			else {
                //else insert PLUS after the base aliases to restore previous "reverse state"
				insert_it =  first.insert(insert_it, t_translation::PLUS);
			}

			first.insert(insert_it, second.begin(), second.end());

			break;
		}
	}

}
