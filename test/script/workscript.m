if assigned printVersion then print "test version of test script"; quit; end if;

if not assigned processId then error "variable processId should be assigned to unique string"; end if;
if not assigned dataFile then error "variable dataFile should point to a valid data file"; end if;
if not assigned outputPath then error "variable outputPath should point to a directory to contain the output"; end if;
if not assigned megabytes then error  "variable megabytes should indicate a memory limit in MB (or 0 for no limit)"; end if;

SetMemoryLimit(StringToInteger(megabytes)*1024*1024);
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

WriteLine:=procedure(d,X,Y,data,outputPath)
	WriteToOutputFile(Sprint(d) cat ";" cat X cat ";" cat Y cat ";Odin;" cat data cat ";6;7;8",outputPath);
end procedure;


ComputeAndWriteToFile:=procedure(fileName,outputPath)
	file:=Open(fileName,"r");
	line:=Gets(file);
	while not IsEof(line) do
		d,X,Y:=ReadLine(line);
		WriteLine(d,X,Y,Sprint(d) cat X cat Y,outputPath);
		line:=Gets(file);
	end while;
end procedure;

ComputeAndWriteToFile(dataFile,outputPath);
quit;
