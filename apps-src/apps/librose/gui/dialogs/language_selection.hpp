/* $Id: language_selection.hpp 48440 2011-02-07 20:57:31Z mordante $ */
/*
   Copyright (C) 2008 - 2011 by Mark de Wever <koraq@xs4all.nl>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_DIALOGS_LANGUAGE_SELECTION_HPP_INCLUDED
#define GUI_DIALOGS_LANGUAGE_SELECTION_HPP_INCLUDED

#include "gui/dialogs/dialog.hpp"

namespace gui2 {

class tlanguage_selection : public tdialog
{
public:
	tlanguage_selection() {}

private:

	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const override;

	/** Inherited from tdialog. */
	void pre_show() override;

	/** Inherited from tdialog. */
	void post_show() override;
};

} // namespace gui2

#endif

