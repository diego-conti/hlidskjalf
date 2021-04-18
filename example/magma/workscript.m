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

load "magma/hliðskjálflayer.m";
if assigned printVersion then print "1.1"; quit; end if;

ReadLine:=function(line)
	components:=Split(line,";");
	if #components ne 3 then error "Each line in datafile should have the form d;X;Y ", components; end if;
	return StringToInteger(components[1]), components[2],components[3];
end function;

WriteLine:=procedure(primaryInput1,secondaryInput1,secondaryInput2,output)
	WriteComputation(Sprint(primaryInput1) cat ";" cat secondaryInput1 cat ";" cat secondaryInput2 cat ";" cat output);
end procedure;

//return a string representing the output of the computation. In this example, returns the sum of the primary input with the lengths of the secondary inputs
Compute:=function(primaryInput1,secondaryInput1,secondaryInput2)
	return Sprint(primaryInput1+#secondaryInput1+#secondaryInput2);
end function;

ComputeAndWriteToStdout:=procedure(fileName)
	file:=Open(fileName,"r");
	line:=Gets(file);
	while not IsEof(line) do
		primaryInput1,secondaryInput1,secondaryInput2:=ReadLine(line);
		output:=Compute(primaryInput1,secondaryInput1,secondaryInput2);
		WriteLine(primaryInput1,secondaryInput1,secondaryInput2,output);
		line:=Gets(file);
	end while;
end procedure;

ComputeAndWriteToStdout(dataFile);
quit;
