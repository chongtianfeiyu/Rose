/* $Id: game_errors.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2003 by David White <dave@whitevine.net>
   Copyright (C) 2005 - 2012 by Yann Dirson <ydirson@altern.org>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef GAME_ERRORS_HPP_INCLUDED
#define GAME_ERRORS_HPP_INCLUDED

#include "exceptions.hpp"
#include "lua_jailbreak_exception.hpp"

namespace game {

struct mp_server_error : public error {
	mp_server_error(const std::string& msg) : error("MP server error: " + msg) {}
};

/**
 * Error used when game loading fails.
 */
struct load_game_failed : public error {
	load_game_failed() {}
	load_game_failed(const std::string& msg) : error("load_game_failed: " + msg) {}
};

/**
 * Error used when game saving fails.
 */
struct save_game_failed : public error {
	save_game_failed() {}
	save_game_failed(const std::string& msg) : error("save_game_failed: " + msg) {}
};

/**
 * Error used for any general game error, e.g. data files are corrupt.
 */
struct game_error : public error {
	game_error(const std::string& msg) : error("game_error: " + msg) {}
};

/**
 * Exception used to signal that the user has decided to abort a game,
 * and to load another game instead.
 */
class load_game_exception
	: public lua_jailbreak_exception
{
public:

	load_game_exception()
		: lua_jailbreak_exception()
	{
	}

	load_game_exception(
			  const std::string& game_
			, const bool show_replay_
			, const bool cancel_orders_)
		: lua_jailbreak_exception()
	{
		game = game_;
		show_replay = show_replay_;
		cancel_orders = cancel_orders_;
	}

	static std::string game;
	static bool show_replay;
	static bool cancel_orders;

private:

	IMPLEMENT_LUA_JAILBREAK_EXCEPTION(load_game_exception)
};

}

#endif
