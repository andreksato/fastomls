# Fast Obstruction Map Local Search (FOMLS)

FOMLS is a C++ written solver for the two-open dimension nesting problem using penetration-fit raster and obstruction map.

## Dependencies

To compile the solver the Qt version 5 library is required. Furthermore, the CUDA libraries are necessary for the pre-processor.

## Usage

The FOMLS package includes the pre-processor, solver and instances.
The pre-processor is needed to generate the no-fit and inner-fit polygons from the instances.
Finally, the output from the pre-processor is the input for the solver.

### Pre-processor

The puzzle file and the options file are used to execute the pre-processor. Both can be found in the instance folder, for each instance.

Example:

```bash
fastomls_preprocessor ./instance/spp/Dagli/puzzle.txt output_folder --options-file ./instance/spp//Dagli/parameters.txt
```

where

- output_folder: path to output folder where the preprocessed data will be generated

### Solver

The pre-processor generates the nfp/ifp images and an XML file, which is the input for the solver.

Example:

```bash
fastomls_solver instance_folder/dagli.xml --initial bottomleft --method gls --duration 600 --strippacking --rectpacking square --result outlog.dat --layout bestSol.xml --appendseed
```

where:

- instance_folder: path to the preprocessed data folder
- outlog.dat: path to the output file containing execution details
- bestSol.xml: path to the output file containing all the feasible solutions found in ESICUP XML format
- rectpacking: options are: `square` (for the FOMLS|SqPP), `random` (for the random-FOMLS|RPP) and `bagpipe` (for the ratio-FOMLS|RPP).

Also, a file named `compiledResults.txt` containing the final solution of all executions will be generated.

### Output file formats

#### compiledResults.txt

The `compiledResults.txt` file contains data of the final state of all the packing algorithm executions.

Each line contains the following information:

scale | best density | best length | best height | best area | best elapsed time | best iteration | total elapsed time | total iterations | seed

where best refers to the solution with maximum density.

\* the character | was added for visibility; the data is separated by tabs.

#### outlog.dat

The `outlog.dat` file contains data of the execution of single packing algorithm execution.
It can be specified using the `--result` option to the solver.

Each line is generated when a feasible solution is found and contains the following information:

scale | - | length | iteration | elapsed time | iterations per second | seed

out << problem->getScale() << " - " << info.length / problem->getScale() << " " << totalItNum << " " << elapsed << " " << totalItNum / elapsed << " " << threadSeed << "\n";

\* the character | was added for visibility; the data is separated by whitespace.

## Contributing

If you have any ideas, just open an issue and tell me what you think.

If you'd like to contribute, please fork the repository and make changes as you'd like. Pull requests are warmly welcome.

## License

This project is licensed under the GNU GPLv3 License.
