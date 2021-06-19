/* $Id: spacer.hpp 54007 2012-04-28 19:16:10Z mordante $ */
/*
   Copyright (C) 2008 - 2012 by Mark de Wever <koraq@xs4all.nl>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_WIDGETS_SPACER_HPP_INCLUDED
#define GUI_WIDGETS_SPACER_HPP_INCLUDED

#include "gui/widgets/control.hpp"

namespace gui2 {

/**
 * An empty widget.
 *
 * Since every grid cell needs a widget this is a blank widget. This widget can
 * also be used to 'force' sizes.
 *
 * Since we're a kind of dummy class we're always active, our drawing does
 * nothing.
 */
class tspacer : public tcontrol
{
public:
	tspacer()
		: tcontrol(0)
		, best_size_(0, 0)
	{
	}

	/***** ***** ***** ***** layout functions ***** ***** ***** *****/

private:
	/** Inherited from tcontrol. */
	tpoint calculate_best_size() const override
	{
		return best_size_ != tpoint(0, 0)
			? best_size_ : tcontrol::calculate_best_size();
	}
public:

	/***** ***** ***** ***** Inherited ***** ***** ***** *****/

	/** Inherited from tcontrol. */
	void set_active(const bool) override {}

	/** Inherited from tcontrol. */
	bool get_active() const override { return true; }

	/** Inherited from tcontrol. */
	unsigned get_state() const override { return 0; }

	/** Inherited from tcontrol. */
	bool disable_click_dismiss() const override { return false; }


private:

	/** When we're used as a fixed size item, this holds the best size. */
	tpoint best_size_;

	/**
	 * Inherited from tcontrol.
	 *
	 * Since we're always empty the draw does nothing.
	 */
	void impl_draw_background(
			  texture& /*frame_buffer*/
			, int /*x_offset*/
			, int /*y_offset*/) override {}

	/** Inherited from tcontrol. */
	const std::string& get_control_type() const override;
};

tspacer* create_spacer(const std::string& id, int width = 0, int height = 0);

} // namespace gui2

#endif


