/* $Id: toggle_panel.cpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2007 - 2012 by Mark de Wever <koraq@xs4all.nl>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#define GETTEXT_DOMAIN "rose-lib"

#include "gui/auxiliary/widget_definition/toggle_panel.hpp"
#include "gui/widgets/widget.hpp"

namespace gui2 {

ttoggle_panel_definition::ttoggle_panel_definition(const config& cfg)
	: tcontrol_definition(cfg)
{
	load_resolutions<tresolution>(cfg);
}

ttoggle_panel_definition::tresolution::tresolution(const config& cfg)
	: tresolution_definition_(cfg)
	, top_margin(cfg["top_margin"].to_int())
	, bottom_margin(cfg["bottom_margin"].to_int())
	, left_margin(cfg["left_margin"].to_int())
	, right_margin(cfg["right_margin"].to_int())
	, frame(cfg["frame"])
{
/*WIKI
 * @page = GUIWidgetDefinitionWML
 * @order = 1_toggle_panel
 *
 * == Toggle panel ==
 *
 * @begin{parent}{name="gui/"}
 * @begin{tag}{name="toggle_panel_definition"}{min=0}{max=-1}{super="generic/widget_definition"}
 * The definition of a toggle panel. A toggle panel is like a toggle button, but
 * instead of being a button it's a panel. This means it can hold multiple child
 * items.
 *
 * @begin{tag}{name="resolution"}{min=0}{max=-1}{super=generic/widget_definition/resolution}
 * The resolution for a toggle panel also contains the following keys:
 * @begin{table}{config}
 *     top_border & unsigned & 0 &     The size which isn't used for the client
 *                                   area. $
 *     bottom_border & unsigned & 0 &  The size which isn't used for the client
 *                                   area. $
 *     left_border & unsigned & 0 &    The size which isn't used for the client
 *                                   area. $
 *     right_border & unsigned & 0 &   The size which isn't used for the client
 *                                   area. $
 * @end{table}
 *
 * The following states exist:
 * * state_enabled, the panel is enabled and not selected.
 * * state_disabled, the panel is disabled and not selected.
 * * state_focussed, the mouse is over the panel and not selected.
 *
 * * state_enabled_selected, the panel is enabled and selected.
 * * state_disabled_selected, the panel is disabled and selected.
 * * state_focussed_selected, the mouse is over the panel and selected.
 * @begin{tag}{name="state_enabled"}{min=0}{max=1}{super="generic/state"}
 * @end{tag}{name="state_enabled"}
 * @begin{tag}{name="state_disabled"}{min=0}{max=1}{super="generic/state"}
 * @end{tag}{name="state_disabled"}
 * @begin{tag}{name="state_focussed"}{min=0}{max=1}{super="generic/state"}
 * @end{tag}{name="state_focussed"}
 * @begin{tag}{name="state_enabled_selected"}{min=0}{max=1}{super="generic/state"}
 * @end{tag}{name="state_enabled_selected"}
 * @begin{tag}{name="state_disabled_selected"}{min=0}{max=1}{super="generic/state"}
 * @end{tag}{name="state_disabled_selected"}
 * @begin{tag}{name="state_focussed_selected"}{min=0}{max=1}{super="generic/state"}
 * @end{tag}{name="state_focussed_selected"}
 * @end{tag}{name="resolution"}
 * @end{tag}{name="toggle_panel_definition"}
 * @end{parent}{name="gui/"}
 */
	VALIDATE(!(left_margin % 4) && !(right_margin % 4) && !(top_margin % 4) && !(bottom_margin % 4), null_str);
	left_margin = cfg_2_os_size(left_margin);
	right_margin = cfg_2_os_size(right_margin);
	top_margin = cfg_2_os_size(top_margin);
	bottom_margin = cfg_2_os_size(bottom_margin);

	// Note the order should be the same as the enum tstate in toggle_panel.hpp.
	state.push_back(tstate_definition(cfg.child("state_enabled")));
	state.push_back(tstate_definition(cfg.child("state_disabled")));
	state.push_back(tstate_definition(cfg.child("state_focussed")));

	state.push_back(tstate_definition(cfg.child("state_enabled_selected")));
	state.push_back(tstate_definition(cfg.child("state_disabled_selected")));
	state.push_back(tstate_definition(cfg.child("state_focussed_selected")));
}

} // namespace gui2

