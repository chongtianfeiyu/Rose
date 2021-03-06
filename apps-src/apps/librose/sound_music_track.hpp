/* $Id: sound_music_track.hpp 46186 2010-09-01 21:12:38Z silene $ */
/*
   Copyright (C) 2003 - 2010 by David White <dave@whitevine.net>
   Copyright (C) 2009 - 2010 by Ignacio R. Morelle <shadowm2006@gmail.com>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef SOUND_MUSIC_TRACK_HPP_INCLUDED
#define SOUND_MUSIC_TRACK_HPP_INCLUDED

#include <string>

class config;

namespace sound {

/**
 * Internal representation of music tracks.
 */
class music_track
{
public:
	music_track();
	music_track(const config& node);
	music_track(const std::string& v_name);
	void write(config& parent_node, bool append);

	bool valid() const { return file_path_.empty() != true; }

	bool append() const { return append_; }
	bool immediate() const { return immediate_; }
	bool play_once() const { return once_; }
	int ms_before() const { return ms_before_; }
	int ms_after()  const { return ms_after_;  }

	const std::string& file_path() const { return file_path_; }
	const std::string& id() const { return id_; }

	void set_play_once(bool v) { once_ = v; }

private:
	void resolve();

	std::string id_;
	std::string file_path_;

	int ms_before_, ms_after_;

	bool once_;
	bool append_;
	bool immediate_;
};

} /* end namespace sound */

inline bool operator==(const sound::music_track& a, const sound::music_track& b) {
	return a.file_path() == b.file_path();
}
inline bool operator!=(const sound::music_track& a, const sound::music_track& b) {
	return a.file_path() != b.file_path();
}

#endif /* ! SOUND_MUSIC_TRACK_HPP_INCLUDED */
