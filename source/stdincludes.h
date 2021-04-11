/***************************************************************************
	Copyright (C) 2021 by Diego Conti, diego.conti@unimib.it

	This file is part of hliðskjálf.
	Hliðskjálf is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef STD_INCLUDES_H
#define STD_INCLUDES_H

#include <string>
#include <vector>
#include <list>
#include <set>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <unordered_set>

using std::string;
using namespace std::literals;
using std::to_string;
using std::stoi;

using std::unique_ptr;
using std::make_unique;
using std::optional;
using std::make_optional;
using std::nullopt;

using std::max;
using std::min;

using std::vector;
using std::list;
using std::set;
using std::unordered_set;
using std::hash;
using std::deque;
using std::map;
using std::pair;

using std::ifstream;
using std::ofstream;
using std::ostream;
using std::istream;
using std::cout;
using std::endl;

using megabytes = int;

#endif
