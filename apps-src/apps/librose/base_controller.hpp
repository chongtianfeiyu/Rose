/* $Id: base_controller.hpp 47608 2010-11-21 01:56:29Z shadowmaster $ */
/*
   Copyright (C) 2006 - 2010 by Joerg Hinrichs <joerg.hinrichs@alice-dsl.de>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef LIBROSE_BASE_CONTROLLER_H_INCLUDED
#define LIBROSE_BASE_CONTROLLER_H_INCLUDED

#include "global.hpp"

#include "key.hpp"
#include "gui/widgets/button.hpp"

class CVideo;
class base_map;
class display;

namespace gui2 {
class treport;
}

namespace events {
class mouse_handler_base;
}

class base_controller: public tevent_dispatcher
{
public:
	enum DIRECTION {UP, DOWN, LEFT, RIGHT, NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST};

	base_controller(int ticks, const config& app_cfg, CVideo& video);
	virtual ~base_controller();

	void initialize(int initial_zoom);
	int main_loop();

	virtual void app_create_display(int initial_zoom) = 0;
	virtual void app_post_initialize() = 0;

	virtual void set_zoom(int new_zoom) {}

	void play_slice(bool is_delay_enabled = true);

	bool in_browse() const { return browse_; }
	base_unit* mouseover_unit() const { return mouseover_unit_; }
	/**
	 * Get a reference to a mouse handler member a derived class uses
	 */
	virtual events::mouse_handler_base& get_mouse_handler_base() = 0;

	virtual bool app_in_context_menu(const std::string& id) const { return true; }
	virtual bool actived_context_menu(const std::string& id) const;
	virtual void prepare_show_menu(gui2::tbutton& widget, const std::string& id, int width, int height) const {}

	void execute_command(int command, const std::string& minor);

	virtual void app_execute_command(int command, const std::string& sparam);
	virtual bool app_can_execute_command(int command, const std::string& sparam) const { return true; }

	/**
	 * Get a reference to a display member a derived class uses
	 */
	virtual display& get_display() = 0;
	virtual const display& get_display() const = 0;

	/**
	 * Get a reference to a base_unit member a derived class uses
	 */
	virtual base_map& get_units() = 0;
	virtual const base_map& get_units() const = 0;

	const config& app_cfg() const { return app_cfg_; }
	int quit_mode() const { return quit_mode_; }

	void handle_mouse_leave_main_map(const int x, const int y, const int up_result);

protected:
	/**
	 * Called by play_slice after events:: calls, but before processing scroll
	 * and other things like menu.
	 */
	virtual void slice_before_scroll();

	/**
	 * Called at the very end of play_slice
	 */
	virtual void slice_end();

	/**
	 * Derived classes should override this to return false when arrow keys
	 * should not scroll the map, hotkeys not processed etc, for example
	 * when a textbox is active
	 * @returns true when arrow keys should scroll the map, false otherwise
	 */
	virtual bool have_keyboard_focus();

	/**
	 * Handle scrolling by keyboard and moving mouse near map edges
	 * @see is_keyboard_scroll_active
	 * @return true when there was any scrolling, false otherwise
	 */
	bool handle_scroll(CKey& key, int mousex, int mousey, int mouse_flags);

	/**
	 * Process mouse- and keypress-events from SDL.
	 * Not virtual but calls various virtual function to allow specialized
	 * behaviour of derived classes.
	 */
	bool mini_pre_handle_event(const SDL_Event& event) override;
	void mini_handle_event(const SDL_Event& event) override;

	/**
	 * Process keydown (only when the general map display does not have focus).
	 */
	virtual void process_focus_keydown_event(const SDL_Event& event);

	/**
	 * Process keydown (always).
	 * Overridden in derived classes
	 */
	virtual void process_keydown_event(const SDL_Event& event);

	/**
	 * Process keyup (always).
	 * Overridden in derived classes
	 */
	virtual void process_keyup_event(const SDL_Event& event);

	virtual void pinch_event(bool out);

	// override tevent_dispatcher
	bool finger_coordinate_valid(int x, int y) const override;
	bool mouse_wheel_coordinate_valid(int x, int y) const override;
	void handle_swipe(const int x, const int y, const int xlevel, const int ylevel) override;
	void handle_pinch(const int x, const int y, const bool out) override;
	void handle_mouse_down(const SDL_MouseButtonEvent& button) override;
	void handle_mouse_up(const SDL_MouseButtonEvent& button) override;
	void handle_mouse_motion(const SDL_MouseMotionEvent& motion) override;

private:
	void init_music(const config& game_config);

	virtual void app_left_mouse_down(const int x, const int y, const bool minimap) {}
	virtual void app_left_mouse_up(const int x, const int y, const bool click) {}
	virtual void app_mouse_leave_main_map(const int x, const int y, const bool up_result) {}

	virtual void app_right_mouse_down(const int x, const int y) {}
	virtual void app_right_mouse_up(const int x, const int y) {}

	virtual bool app_mouse_motion(const int x, const int y, const bool minimap) { return true; }

protected:
	const config& app_cfg_;
	CVideo& video_;

	const int ticks_;
	CKey key_;
	bool browse_;
	bool scrolling_;

	tpoint drag_from_;
	base_unit* mouseover_unit_;

	// finger motion result to scroll
	bool finger_motion_scroll_;
	int finger_motion_direction_;

	// Quit main loop flag
	bool do_quit_;
	int quit_mode_;
};


#endif
