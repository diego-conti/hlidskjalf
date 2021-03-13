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

load "example/magma/readfromdb.m";

if not assigned PATH_TO_DB then error "variable PATH_TO_DB should point to the directory containing the database"; end if;

test:=procedure(primaryInput,secondaryInput1,secondaryInput2)
	values:=[Sprint(secondaryInput1),secondaryInput2];
	test,entry:=fromDb(PATH_TO_DB,primaryInput,values);
	print primaryInput,secondaryInput1, secondaryInput2, "->",entry;
end procedure;

test(2,94,"b");
test(2,94,"d");
test(1,1,"a");
test(1,1,"c");
test(1,2,"a");
test(1,2,"c");

quit;
