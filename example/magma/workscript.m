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

if assigned printVersion then print "1.0"; quit; end if;

if not assigned processId then error "variable processId should be assigned to unique string"; end if;
if not assigned dataFile then error "variable dataFile should point to a valid data file"; end if;
if not assigned outputPath then error "variable outputPath should point to a directory to contain the output"; end if;
if not assigned memory then error  "variable memory should indicate a memory limit in GB or 0 for no limit"; end if;

SetMemoryLimit(StringToInteger(memory)*1024*1024*1024);
SetQuitOnError(true);
SetColumns(1024);

WriteToOutputFile:=procedure(riga,outputPath)
	Write(outputPath cat "/" cat Sprint(processId) cat ".work",riga);
end procedure;

ReadLine:=function(line)
	components:=Split(line,";");
	if #components ne 3 then error "Each line in datafile should have the form d;X;Y ", components; end if;
	return StringToInteger(components[1]), components[2],components[3];
end function;

WriteLine:=procedure(primaryInput1,secondaryInput1,secondaryInput2,output,outputPath)
	WriteToOutputFile(Sprint(primaryInput1) cat ";" cat secondaryInput1 cat ";" cat secondaryInput2 cat ";" cat output,outputPath);
end procedure;

//return a string representing the output of the computation. In this example, returns the sum of the primary input with the lengths of the secondary inputs
Compute:=function(primaryInput1,secondaryInput1,secondaryInput2)
	return Sprint(primaryInput1+#secondaryInput1+#secondaryInput2);
end function;

ComputeAndWriteToFile:=procedure(fileName,outputPath)
	file:=Open(fileName,"r");
	line:=Gets(file);
	while not IsEof(line) do
		primaryInput1,secondaryInput1,secondaryInput2:=ReadLine(line);
		output:=Compute(primaryInput1,secondaryInput1,secondaryInput2);
		WriteLine(primaryInput1,secondaryInput1,secondaryInput2,output,outputPath);
		line:=Gets(file);
	end while;
end procedure;

ComputeAndWriteToFile(dataFile,outputPath);
quit;
