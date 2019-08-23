#ifndef _jschema_H
#define _jschema_H

#include <memory>
#include "json_fifo.h"

/**
  * jschema.h
  * -------------------------
  * Code for generating a JSON Schema that can be used to validate the harmonics schemas understood
  * by xtwsd.
  * -------------------------
  * @author Joel Kozikowski
  */

//  (C) 2019 Joel Kozikowski
//
//  This file is part of xtwsd.
//
//  xtwsd is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  xtwsd is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Foobar.  If not, see <https://www.gnu.org/licenses/>.






/**
 * Populates the specified json object with the JSON Schema for harmonics data used
 * in the jsonxt.h module.  If the XTide data file can not be opened, the passed
 * object will remain unchanged.
 */
extern void getJsonSchema(json& schema);


#endif