/* $Id: stack.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2009 - 2012 by Mark de Wever <koraq@xs4all.nl>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_AUXILIARY_WINDOW_BUILDER_STACK_HPP_INCLUDED
#define GUI_AUXILIARY_WINDOW_BUILDER_STACK_HPP_INCLUDED

#include "gui/auxiliary/window_builder/control.hpp"
#include "gui/widgets/stack.hpp"

#include <vector>

namespace gui2 {

namespace implementation {

struct tbuilder_stack
	: public tbuilder_control
{
	explicit tbuilder_stack(const config& cfg);

	twidget* build () const;

	/** The builders for all layers of the stack .*/
	std::vector<std::pair<tbuilder_grid_const_ptr, unsigned> > stack;

	tstack::tmode mode;
};

} // namespace implementation

} // namespace gui2

#endif

