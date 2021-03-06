/* $Id: spacer.cpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#define GETTEXT_DOMAIN "rose-lib"

#include "gui/auxiliary/window_builder/spacer.hpp"

#include "config.hpp"
#include "gui/widgets/spacer.hpp"

namespace gui2 {

namespace implementation {

tbuilder_spacer::tbuilder_spacer(const config& cfg)
	: tbuilder_control(cfg)
{
}

twidget* tbuilder_spacer::build() const
{
	tspacer* widget = new tspacer();

	init_control(widget);

	// Window builder: placed spacer 'id' with definition 'definition'.

	return widget;
}

} // namespace implementation

} // namespace gui2

/*WIKI_MACRO
 * @begin{macro}{spacer_description}
 *
 *        A spacer is a dummy item to either fill in a widget since no empty
 *        items are allowed or to reserve a fixed space.
 * @end{macro}
 */


/*WIKI
 * @page = GUIWidgetInstanceWML
 * @order = 2_spacer
 * @begin{parent}{name="gui/window/resolution/grid/row/column/"}
 * @begin{tag}{name="spacer"}{min=0}{max=-1}{super="generic/widget_instance"}
 * == Spacer ==
 *
 * @macro = spacer_description
 *
 * If either the width or the height is non-zero the spacer functions as a
 * fixed size spacer.
 *
 * @begin{table}{config}
 *     width & f_unsigned & 0 &          The width of the spacer. $
 *     height & f_unsigned & 0 &         The height of the spacer. $
 * @end{table}
 *
 * The variable available are the same as for the window resolution see
 * /wiki/GUIToolkitWML#Resolution_2 for the list of
 * items.
 * @end{tag}{name="spacer"}
 * @end{parent}{name="gui/window/resolution/grid/row/column/"}
 */

