/* $Id: toggle_button.cpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#include "gui/auxiliary/widget_definition/toggle_button.hpp"

namespace gui2 {

ttoggle_button_definition::ttoggle_button_definition(const config& cfg)
	: tcontrol_definition(cfg)
{
	load_resolutions<tresolution>(cfg);
}

ttoggle_button_definition::tresolution::tresolution(const config& cfg)
	: tresolution_definition_(cfg)
{
/*WIKI
 * @page = GUIWidgetDefinitionWML
 * @order = 1_toggle_button
 *
 * == Toggle button ==
 *
 * The definition of a toggle button.
 *
 * The following states exist:
 * * state_enabled, the button is enabled and not selected.
 * * state_disabled, the button is disabled and not selected.
 * * state_focussed, the mouse is over the button and not selected.
 *
 * * state_enabled_selected, the button is enabled and selected.
 * * state_disabled_selected, the button is disabled and selected.
 * * state_focussed_selected, the mouse is over the button and selected.
 * @begin{parent}{name="gui/"}
 * @begin{tag}{name="toggle_button_definition"}{min=0}{max=-1}{super="generic/widget_definition"}
 * @begin{tag}{name="resolution"}{min=0}{max=-1}{super="generic/widget_definition/resolution"}
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
 * @end{tag}{name="toggle_button_definition"}
 * @end{parent}{name="gui/"}
 */

	// Note the order should be the same as the enum tstate in
	// toggle_button.hpp.
	state.push_back(tstate_definition(cfg.child("state_enabled")));
	state.push_back(tstate_definition(cfg.child("state_disabled")));
	state.push_back(tstate_definition(cfg.child("state_focussed")));

	state.push_back(tstate_definition(cfg.child("state_enabled_selected")));
	state.push_back(tstate_definition(cfg.child("state_disabled_selected")));
	state.push_back(tstate_definition(cfg.child("state_focussed_selected")));
}

} // namespace gui2

