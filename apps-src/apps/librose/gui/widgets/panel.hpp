/* $Id: panel.hpp 54007 2012-04-28 19:16:10Z mordante $ */
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

#ifndef GUI_WIDGETS_PANEL_HPP_INCLUDED
#define GUI_WIDGETS_PANEL_HPP_INCLUDED

#include "gui/widgets/container.hpp"

namespace gui2 {

/**
 * Visible container to hold multiple widgets.
 *
 * This widget can draw items beyond the widgets it holds and in front of them.
 * A panel is always active so these functions return dummy values.
 */
class tpanel : public tcontainer_
{

public:

	/**
	 * Constructor.
	 *
	 * @param canvas_count        The canvas count for tcontrol.
	 */
	explicit tpanel(const unsigned canvas_count = 1) ;

	/** Inherited from tcontrol. */
	bool get_active() const override { return true; }

	/** Inherited from tcontrol. */
	unsigned get_state() const override { return 0; }

	void set_hole_variable(int left, int right);

	/** Inherited from tcontrol. */
	void update_canvas() override;

	/** Inherited from tcontrol. */
	void impl_draw_background(
			  texture& frame_buffer
			, int x_offset
			, int y_offset) override;

	void set_can_invalidate_layout() { can_invalidate_layout_ = true; }

protected:
	/** Inherited from tcontainer_. */
	void layout_children() override;

private:
	/** Inherited from tcontainer_. */
	tspace4 mini_conf_margin() const override;
	std::string mini_default_border() const override { return "default_border"; }

	bool can_invalidate_layout(const twidget& widget) const override;
	void invalidate_layout(twidget* widget) override;

private:
	tgrid grid_;

	int hole_left_;
	int hole_right_;

	bool can_invalidate_layout_;
	bool need_layout_;

	/** Inherited from tcontainer_. */
	void set_self_active(const bool /*active*/) override {}

	/** Inherited from tcontrol. */
	const std::string& get_control_type() const override;
};

} // namespace gui2

#endif


