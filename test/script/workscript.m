load "../../magma/hliðskjálflayer.m";

if assigned printVersion then print "test version of test script"; quit; end if;

ReadLine:=function(line)
	components:=Split(line,";");
	if #components ne 3 then error "Each line in datafile should have the form d;X;Y ", components; end if;
	return StringToInteger(components[1]), components[2],components[3];
end function;

WriteLine:=procedure(d,X,Y,data)
	WriteComputation(Sprint(d) cat ";" cat X cat ";" cat Y cat ";Odin;" cat data cat ";6;7;8");
end procedure;

ComputeAndWriteToStdout:=procedure(fileName)
	file:=Open(fileName,"r");
	line:=Gets(file);
	while not IsEof(line) do
		d,X,Y:=ReadLine(line);
		WriteLine(d,X,Y,Sprint(d) cat X cat Y);
		line:=Gets(file);
	end while;
end procedure;

ComputeAndWriteToStdout(dataFile);
quit;
