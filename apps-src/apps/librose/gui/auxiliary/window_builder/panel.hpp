/* $Id: panel.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#ifndef GUI_AUXILIARY_WINDOW_BUILDER_PANEL_HPP_INCLUDED
#define GUI_AUXILIARY_WINDOW_BUILDER_PANEL_HPP_INCLUDED

#include "gui/auxiliary/window_builder/control.hpp"

namespace gui2 {

namespace implementation {

struct tbuilder_panel
	: public tbuilder_control
{
	explicit tbuilder_panel(const config& cfg);

	twidget* build () const;

	tbuilder_grid_ptr grid;

	int left_margin;
	int right_margin;
	int top_margin;
	int bottom_margin;
};

} // namespace implementation

} // namespace gui2

#endif

