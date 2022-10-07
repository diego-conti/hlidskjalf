# Hliðskjálf
`Hliðskjálf` is a program to parallelize computations in [Magma](http://magma.maths.usyd.edu.au/magma/). 

The user must provide

- a *work script*, namely a Magma script that performs the actual computational work. The work script takes a list of computations as an input and outputs a CSV file where each row contains both the input and the output for a specific computation.
- a *computations file*, namely a CSV file whose rows represent computations to be performed.
- a *schema file*, which defines the format of both the work script output and the computations file; in particular, it defines which columns correspond to computation input, which to output, and which should be ignored. In addition, the schema file may specify that some columns of the computation file represent ranges; this means that a row in the computation file is converted into a sequence of computations to be fed to the work script, one for each integer in the specified range.

## Compiling, installing and testing

`Hliðskjálf` requires [CMake](https://cmake.org) and the [Boost libraries](https://www.boost.org) to compile; it requires [Magma](http://magma.maths.usyd.edu.au/magma/) to run. `Hliðskjálf` has been tested on CentOS Linux 7, using gcc 8.3.1, boost 1.71, cmake 3.24.2 and Magma 2.24-5.

To compile, run

	mkdir build
	cd build
	cmake ..
	cmake --build .
	
This creates two executables in the `build` directory.

To install, run 

	cd build
	cmake --install . --prefix=the/path/I/choose

This installs the executables `yggdrasill` and `hliðskjálf` to the path of your choice.

To test that everything works, run
	
	cd build
	ctest --test-dir test
	
The directory *example* contains an example which can be run using
	
	example/sh/run.sh

## Components
There are two components to this program.

`Yggdrasill` reads the output of the work script and stores it into a database. The current implementation of the database is just a sequence of CSV files, with name parametrized by the primary input column. Each row contains the secondary input and the output. The database can then be read by a Magma script, see `examples/readfromdb.m` for an example.

`Hliðskjálf` runs the work script. The computation is run in parallel, by distributing the computations to be done among different Magma processes. Each process is instructed to run with a given memory limit, and possibly with a time limit; if Magma terminates before finishing (because time or memory run out), Hliðskjálf tries to assign the offending computation to a process with a higher memory limit as soon as one becomes available. The distribution of memory among processes is changed automatically as computations progress. Computations which cannot be completed even by increasing the memory limit are skipped, and their input values stored into a *valhalla* file. Hliðskjálf ensures that computations are not repeated by reading the files in the work script output directory, and eliminating the corresponding computations. In addition, hliðskjálf can be instructed to eliminate the computations in an existing database.

If `--base-timeout` is also specified, processes are also assigned a time limit. This limit is increased alongside with the memory limit, proportionally, when computations are repeated.

## Schema file

A text file taking e.g. the following form

	columns 7 {
		output 6
		output 5
	}
	inputcolumns 4 {
		textinputcolumn 3
		textinputcolumn 2
	}

This schema declares that each row of the work script output contains 7 columns. Columns 6 and 5 represent computation output; columns 4,2,3 the input; columns 1 and 7 are ignored. Column 4 is an integer, which represents the *primary* input to the computation; columns 2 and 3 are treated as arbitrary text. 

The format of the computations file is inferred from this definition: each row contains three columns, representing in this order: the primary input (an integer), the secondary input corresponding to column 3 in the work script output, and the secondary input corresponding to column 2 in the work script output. In practice, it is typically simpler to respect the order in the definition, so the above could be rewritten as 

	columns 7 {
		output 5
		output 6
	}
	inputcolumns 1 {
		textinputcolumn 2
		textinputcolumn 3
	}

In this case, the first three columns of the computation file correspond to the first three columns of the work script output, taken in the same order.

### Range columns
Input columns may contain ranges, as in the following:

	inputcolumns 1 {
		rangeinputcolumn 2
		textinputcolumn 3
	}

This declare that rows in the computation files will take the form 

	d;a..b;TEXT. 

Here, d,a,b are integers, and TEXT is an arbitrary string. This line is interpreted as a list of computations to be passed to the work script, namely

	d;a;TEXT
		.
		.
		.
	d;b;TEXT

### Replacement rules
Output column specification may contain replacement rules, as in 

	columns 7 {
		output 5
		replacement {
			match Odin
			with Alfaðir
		}
	}

This has the effect of replacing every occurrence of Odin in column 5 with Alfaðir, when `yggdrasill` reads work script output to update the database. To define more than one replacement rule, use curly braces, as in 

	columns 7 {
		output 5 {
			replacement {
				match gold
				with Sifjar haddr
			}
			replacement {
				match sky
				with Ymis haus
			}
		}			
	}

### Omit rules
The schema file can also contain patterns of the form:

	omitrules {
		condition {
			column 6
			contains Huginn
		}
		condition {
			column 5
			match Muninn	
		}
	}

The effect of this declaration is to ignore, when `yggdrasill` reads work script output, every row such that either column 6 contains the string Huginn or column 5 matches exactly the string Muninn. The corresponding entries will not be inserted into the database.


## Work script

The work script is a Magma program that accepts the following parameters:

* printVersion. If set, then the script only prints a string identifying the script version and quits;
* dataFile. Path of a file containing the list of computations to be performed.
* megabytes. Memory limit in MB. The script should honour this limit by invoking 

		SetMemoryLimit(StringToInteger(megabytes)*1024*1024)

The dataFile is a CSV generated by hliðskjálf, containing the list of computatations. ; is used as a separator. The first entry in the row is the primary input; the others are the secondary inputs (in the order in which they appear in the schema definition).
The work script should `load` the file included in `magma/hliðskjálflayer.m` and use the function `WriteComputation` defined therein to print each line of the output. The output format is CSV, with ; as a separator, with each row containing both the input and the output of a computation. The precise format should respect the schema definition.
