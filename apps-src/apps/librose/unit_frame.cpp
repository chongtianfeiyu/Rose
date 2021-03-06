/* $Id: unit_frame.cpp 47820 2010-12-05 18:08:51Z mordante $ */
/*
   Copyright (C) 2006 - 2010 by Jeremy Rosen <jeremy.rosen@enst-bretagne.fr>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/** @file */

#include "global.hpp"

#include "display.hpp"
#include "serialization/string_utils.hpp"
#include "unit_frame.hpp"

progressive_string::progressive_string(const std::string& data,int duration) :
	data_(),
	input_(data)
{
	const std::vector<std::string> first_pass = utils::split(data);
	const int colons = (int)first_pass.size();

	int total_explicit_time = 0, explicit_colon = 0;
	std::vector<std::string>::const_iterator tmp;
	for (tmp = first_pass.begin(); tmp != first_pass.end() ; ++ tmp) {
		std::vector<std::string> second_pass = utils::split(*tmp, ':');

		int explicit_time = (second_pass.size() > 1)? atoi(second_pass[1].c_str()): 0;
		VALIDATE(explicit_time >= 0, null_str);
		if (explicit_time) {
			explicit_colon ++;
		}
		total_explicit_time += explicit_time;

		data_.push_back(std::pair<std::string,int>(second_pass[0], explicit_time));
	}

	VALIDATE(total_explicit_time <= duration, null_str);
	if (explicit_colon != colons) {
		// 2. calculate average_time, and set it to no explicit itmes.
		const int average_time = (duration - total_explicit_time) / (colons - explicit_colon);
		for (std::vector<std::pair<std::string, int> >::iterator it = data_.begin(); it != data_.end(); ++ it) {
			if (!it->second) {
				it->second = average_time;
			}
		}
	}
}

int progressive_string::duration() const
{
	int total =0;
	std::vector<std::pair<std::string,int> >::const_iterator cur_halo;
	for(cur_halo = data_.begin() ; cur_halo != data_.end() ; ++cur_halo) {
		total += cur_halo->second;
	}
	return total;

}

const std::string& progressive_string::get_current_element(int current_time)const
{
	int time = 0;
	unsigned int sub_halo = 0;
	if(data_.empty()) return null_str;
	while(time < current_time&& sub_halo < data_.size()) {
		time += data_[sub_halo].second;
		++sub_halo;

	}
	if(sub_halo) sub_halo--;
	return data_[sub_halo].first;
}

template <class T>
progressive_<T>::progressive_(const std::string &data, int duration) :
	data_(),
	input_(data)
{
	int split_flag = utils::REMOVE_EMPTY; // useless to strip spaces
	const std::vector<std::string> comma_split = utils::split(data,',',split_flag);
	const int colons = (int)comma_split.size();

	// 1. calculate total_explicit, item's time that no explicit set to 0.
	int total_explicit_time = 0, explicit_colon = 0;
	std::vector<std::string>::const_iterator com_it = comma_split.begin();
	for (; com_it != comma_split.end(); ++com_it) {
		std::vector<std::string> colon_split = utils::split(*com_it,':',split_flag);
		int explicit_time = (colon_split.size() > 1)? atoi(colon_split[1].c_str()): 0;
		VALIDATE(explicit_time >= 0, null_str);
		if (explicit_time) {
			explicit_colon ++;
		}
		total_explicit_time += explicit_time;

		std::vector<std::string> range = utils::split(colon_split[0],'~',split_flag);
		T range0 = lexical_cast<T>(range[0]);
		T range1 = (range.size() > 1) ? lexical_cast<T>(range[1]) : range0;
		typedef std::pair<T,T> range_pair;
		data_.push_back(std::pair<range_pair,int>(range_pair(range0, range1), explicit_time));
	}

	VALIDATE(total_explicit_time <= duration, null_str);
	if (explicit_colon != colons) {
		// 2. calculate average_time, and set it to no explicit itmes.
		const int average_time = (duration - total_explicit_time) / (colons - explicit_colon);
		for (typename std::vector<std::pair<std::pair<T, T>, int> >::iterator it = data_.begin(); it != data_.end(); ++ it) {
			if (!it->second) {
				it->second = average_time;
			}
		}
	}
}


template <class T>
const T progressive_<T>::get_current_element(int current_time, T default_val) const
{
	int time = 0;
	unsigned int sub_halo = 0;
	int searched_time = current_time;
	if(searched_time < 0) searched_time = 0;
	if(searched_time > duration()) searched_time = duration();
	if(data_.empty()) return default_val;
	while(time < searched_time&& sub_halo < data_.size()) {
		time += data_[sub_halo].second;
		++sub_halo;

	}
	if(sub_halo != 0) {
		sub_halo--;
		time -= data_[sub_halo].second;
	}

	const T first =  data_[sub_halo].first.first;
	const T second =  data_[sub_halo].first.second;

	return T((static_cast<double>(searched_time - time) /
		static_cast<double>(data_[sub_halo].second)) *
		(second - first) + first);
}

template<class T>
int progressive_<T>::duration() const
{
	int total = 0;
	typename std::vector<std::pair<std::pair<T, T>, int> >::const_iterator cur_halo;
	for(cur_halo = data_.begin() ; cur_halo != data_.end() ; ++cur_halo) {
		total += cur_halo->second;
	}
	return total;

}

template <class T>
bool progressive_<T>::does_not_change() const
{
return data_.empty() ||
	( data_.size() == 1 && data_[0].first.first == data_[0].first.second);
}

// Force compilation of the following template instantiations
template class progressive_<int>;
template class progressive_<double>;

frame_parameters frame_parameters::null_param;
const int frame_parameters::default_text_color = 0xFFDDDDDD; // equal font::NORMAL_COLOR

frame_parameters::frame_parameters() :
	duration(0),
	image(),
	image_diagonal(),
	image_horizontal(),
	image_mod(""),
	stext(""),
	halo(""),
	sound(""),
	text(""),
	text_color(0),
	font_size(0),
	blend_with(0),
	blend_ratio(0.0),
	highlight_ratio(1.0),
	offset_x(0),
	offset_y(0),
	submerge(0.0),
	x(0),
	y(0),
	directional_x(0),
	directional_y(0),
	auto_vflip(t_unset),
	auto_hflip(t_unset),
	primary_frame(t_unset),
	drawing_layer(display::LAYER_UNIT_DEFAULT - display::LAYER_UNIT_FIRST),
	area_mode(false),
	sound_filter()
{}

frame_parsed_parameters::frame_parsed_parameters(const config& cfg, const std::string& frame_string, int override_duration)
	: duration_(override_duration? override_duration: cfg[frame_string + "duration"].to_int(1))
	, image_(cfg[frame_string + "image"])
	, image_diagonal_(cfg[frame_string + "image_diagonal"])
	, image_horizontal_(cfg[frame_string + "image_horizontal"])
	, image_mod_(cfg[frame_string + "image_mod"])
	, stext_(cfg[frame_string + "stext"])
	, halo_(cfg[frame_string + "halo"], duration_)
	, sound_(cfg[frame_string + "sound"])
	, text_(cfg[frame_string + "text"])
	, text_color_(frame_parameters::default_text_color)
	, font_size_(cfg[frame_string + "font_size"].to_int(0))
	, blend_with_(0)
	, blend_ratio_(cfg[frame_string + "blend_ratio"], duration_)
	, highlight_ratio_(cfg[frame_string + "alpha"], duration_)
	, offset_x_(cfg[frame_string + "offset_x"], duration_)
	, offset_y_(cfg[frame_string + "offset_y"], duration_)
	, submerge_(cfg[frame_string + "submerge"], duration_)
	, x_(cfg[frame_string + "x"], duration_)
	, y_(cfg[frame_string + "y"], duration_)
	, directional_x_(cfg[frame_string + "directional_x"], duration_)
	, directional_y_(cfg[frame_string + "directional_y"], duration_)
	, auto_vflip_(t_unset)
	, auto_hflip_(t_unset)
	, primary_frame_(t_unset)
	, drawing_layer_(cfg[frame_string + "layer"], duration_)
{
	if (!frame_string.empty()) {
		// not support define duration out of [xxx_frame]. must be additive using these [xxx_frame].
		VALIDATE(!cfg.has_attribute(frame_string + "duration"), null_str);
	}

	if (!cfg.has_attribute(frame_string + "auto_vflip")) {
		auto_vflip_ = t_unset;
	} else if(cfg[frame_string + "auto_vflip"].to_bool()) {
		auto_vflip_ = t_true;
	} else {
		auto_vflip_ = t_false;
	}
	if(!cfg.has_attribute(frame_string + "auto_hflip")) {
		auto_hflip_ = t_unset;
	} else if(cfg[frame_string + "auto_hflip"].to_bool()) {
		auto_hflip_ = t_true;
	} else {
		auto_hflip_ = t_false;
	}
	if(!cfg.has_attribute(frame_string + "primary")) {
		primary_frame_ = t_unset;
	} else if(cfg[frame_string + "primary"].to_bool()) {
		primary_frame_ = t_true;
	} else {
		primary_frame_ = t_false;
	}
	std::vector<std::string> color = utils::split(cfg[frame_string + "text_color"]);
	if (color.size() == 4) {
		text_color_ = display::rgb(atoi(color[1].c_str()), atoi(color[2].c_str()), atoi(color[3].c_str()));
		text_color_ &= 0xffffffff;
	}

	color = utils::split(cfg[frame_string + "blend_color"]);
	if (color.size() == 3) {
		blend_with_ = display::rgb(atoi(color[0].c_str()), atoi(color[1].c_str()), atoi(color[2].c_str()));
	}
}

bool frame_parsed_parameters::does_not_change() const
{
	return halo_.does_not_change() &&
		blend_ratio_.does_not_change() &&
		highlight_ratio_.does_not_change() &&
		offset_x_.does_not_change() &&
		offset_y_.does_not_change() &&
		submerge_.does_not_change() &&
		x_.does_not_change() &&
		y_.does_not_change() &&
		directional_x_.does_not_change() &&
		directional_y_.does_not_change() &&
		drawing_layer_.does_not_change() &&
		stext_.empty();
}
bool frame_parsed_parameters::need_update() const
{
	if(!halo_.does_not_change() ||
			!blend_ratio_.does_not_change() ||
			!highlight_ratio_.does_not_change() ||
			!offset_x_.does_not_change() ||
			!offset_y_.does_not_change() ||
			!submerge_.does_not_change() ||
			!x_.does_not_change() ||
			!y_.does_not_change() ||
			!directional_x_.does_not_change() ||
			!directional_y_.does_not_change() ||
			!drawing_layer_.does_not_change() ) {
			return true;
	}
	return false;
}

const frame_parameters frame_parsed_parameters::parameters(int current_time) const
{
	frame_parameters result;
	result.duration = duration_;
	result.image = image_;
	result.image_diagonal = image_diagonal_;
	result.image_horizontal = image_horizontal_;
	result.image_mod = image_mod_;
	result.stext = stext_;
	result.halo = halo_.get_current_element(current_time);
	result.sound = sound_;
	result.text = text_;
	result.text_color = text_color_;
	result.font_size = font_size_;
	result.blend_with = blend_with_;
	result.blend_ratio = blend_ratio_.get_current_element(current_time);
	result.highlight_ratio = highlight_ratio_.get_current_element(current_time,1.0);
	result.offset_x = offset_x_.get_current_element(current_time,-1000);
	result.offset_y = offset_y_.get_current_element(current_time,-1000);
	result.submerge = submerge_.get_current_element(current_time);
	result.x = x_.get_current_element(current_time);
	result.y = y_.get_current_element(current_time);
	result.directional_x = directional_x_.get_current_element(current_time);
	result.directional_y = directional_y_.get_current_element(current_time);
	result.auto_vflip = auto_vflip_;
	result.auto_hflip = auto_hflip_;
	result.primary_frame = primary_frame_;
	result.drawing_layer = drawing_layer_.get_current_element(current_time,display::LAYER_UNIT_DEFAULT-display::LAYER_UNIT_FIRST);
	return result;
}

void frame_parsed_parameters::override( int duration
		, const std::string& highlight
		, const std::string& blend_ratio
		, Uint32 blend_color
		, const std::string& offset
		, const std::string& layer
		, const std::string& modifiers)
{

	if (!highlight.empty()) {
		highlight_ratio_ = progressive_double(highlight,duration);
	} else if (duration != duration_){
		highlight_ratio_ = progressive_double(highlight_ratio_.get_original(),duration);
	}
	if (!offset.empty()) {
		offset_x_= progressive_double(offset,duration);
	} else if (duration != duration_){
		offset_x_ = progressive_double(offset_x_.get_original(),duration);
	}
	if (!offset.empty()) {
		offset_y_ = progressive_double(offset,duration);
	} else if (duration != duration_){
		offset_y_ = progressive_double(offset_y_.get_original(),duration);
	}
	if (!blend_ratio.empty()) {
		blend_ratio_ = progressive_double(blend_ratio,duration);
		blend_with_  = blend_color;
	} else if (duration != duration_){
		blend_ratio_ = progressive_double(blend_ratio_.get_original(),duration);
	}
	if (!layer.empty()) {
		drawing_layer_ = progressive_int(layer,duration);
	} else if (duration != duration_){
		drawing_layer_ = progressive_int(drawing_layer_.get_original(),duration);
	}
	if (!modifiers.empty()) {
		image_mod_ += modifiers;
	}

	if (duration != duration_) {
		halo_ = progressive_string(halo_.get_original(), duration);
		submerge_=progressive_double(submerge_.get_original(), duration);
		x_=progressive_int(x_.get_original(), duration);
		y_=progressive_int(y_.get_original(), duration);
		directional_x_=progressive_int(directional_x_.get_original(), duration);
		directional_y_=progressive_int(directional_y_.get_original(), duration);
		duration_ = duration;
	}
}

frame_parsed_parameters& frame_parsed_parameters::image(const std::string& image ,const std::string & image_mod)
{
	image_ = image;
	image_mod_ = image_mod;
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::image_diagonal(const image::locator& image_diagonal,const std::string& image_mod)
{
	image_diagonal_ = image_diagonal;
	image_mod_ = image_mod;
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::image_horizontal(const image::locator& image_horizontal,const std::string& image_mod)
{
	image_horizontal_ = image_horizontal;
	image_mod_ = image_mod;
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::sound(const std::string& sound)
{
	sound_=sound;
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::text(const std::string& text,const  Uint32 text_color)
{
	text_=text;
	text_color_=text_color;
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::halo(const std::string &halo, const std::string &x, const std::string& y,const std::string & mod)
{
	halo_ = progressive_string(halo, duration_);
	x_ = progressive_int(x, duration_);
	y_ = progressive_int(y, duration_);
	image_mod_= mod;
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::duration(const int duration)
{
	if (duration != duration_) {
		halo_ = progressive_string(halo_.get_original(), duration);
		blend_ratio_ = progressive_double(blend_ratio_.get_original(), duration);
		highlight_ratio_ = progressive_double(highlight_ratio_.get_original(), duration);
		offset_x_ = progressive_double(offset_x_.get_original(), duration);
		offset_y_ = progressive_double(offset_y_.get_original(), duration);
		submerge_=progressive_double(submerge_.get_original(), duration);
		x_=progressive_int(x_.get_original(), duration);
		y_=progressive_int(y_.get_original(), duration);
		directional_x_=progressive_int(directional_x_.get_original(), duration);
		directional_y_=progressive_int(directional_y_.get_original(), duration);
		drawing_layer_=progressive_int(drawing_layer_.get_original(), duration);
		duration_ = duration;
	}
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::blend(const std::string& blend_ratio,const Uint32 blend_color)
{
	blend_with_ = blend_color;
	blend_ratio_ = progressive_double(blend_ratio, duration_);
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::highlight(const std::string& highlight)
{
	highlight_ratio_ = progressive_double(highlight, duration_);
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::offset(const std::string& offset)
{
	offset_x_ = progressive_double(offset, duration_);
	offset_y_ = progressive_double(offset, duration_);
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::offset(const std::string& offset_x, const std::string& offset_y)
{
	offset_x_ = progressive_double(offset_x, duration_);
	offset_y_ = progressive_double(offset_y, duration_);
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::submerge(const std::string& submerge)
{
	submerge_ = progressive_double(submerge, duration_);
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::x(const std::string& x)
{
	x_ = progressive_int(x, duration_);
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::y(const std::string& y)
{
	y_ = progressive_int(y, duration_);
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::directional_x(const std::string& directional_x)
{
	directional_x_ = progressive_int(directional_x, duration_);
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::directional_y(const std::string& directional_y)
{
	directional_y_ = progressive_int(directional_y, duration_);
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::auto_vflip(const bool auto_vflip)
{
	if (auto_vflip) {
		auto_vflip_ = t_true;
	} else {
		auto_vflip_ = t_false;
	}
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::auto_hflip(const bool auto_hflip)
{
	if (auto_hflip) {
		auto_hflip_ = t_true;
	} else {
		auto_hflip_ = t_false;
	}
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::primary_frame(const bool primary_frame)
{
	if (primary_frame) {
		primary_frame_ = t_true;
	} else {
		primary_frame_ = t_false;
	}
	return *this;
}
frame_parsed_parameters& frame_parsed_parameters::drawing_layer(const std::string& drawing_layer)
{
	drawing_layer_ = progressive_int(drawing_layer, duration_);
	return *this;
}