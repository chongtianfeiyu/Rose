/* $Id: integer_selector.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#ifndef GUI_WIDGETS_INTEGER_SELECTOR_HPP_INCLUDED
#define GUI_WIDGETS_INTEGER_SELECTOR_HPP_INCLUDED

namespace gui2 {

/**
 * Small abstract helper class.
 *
 * Parts of the engine inherit this class so we can have generic
 * widgets to select an integer value.
 */
class tinteger_selector_
{
public:
	virtual ~tinteger_selector_() {}

	/** Sets the selected value. */
	virtual void set_value(const int value) = 0;

	/** Gets the selected value. */
	virtual int get_value() const = 0;

	/** Sets the minimum value. */
	virtual void set_minimum_value(const int value) = 0;

	/** Gets the minimum value. */
	virtual int get_minimum_value() const = 0;

	/** Sets the maximum value. */
	virtual void set_maximum_value(const int value) = 0;

	/** Gets the maximum value. */
	virtual int get_maximum_value() const = 0;
};

} // namespace gui2

#endif
