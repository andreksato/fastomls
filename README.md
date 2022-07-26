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

```bash
fastomls_preprocessor ./Dagli/puzzle.txt . --options-file ./Dagli/parameters.txt --rectpacking square
```

### Solver

The pre-processor generates the nfp/ifp images and an XML file, which is the input for the solver.

```bash
fastomls_solver ./Dagli/dagli.xml . --rectpacking square
```

The rectpacking options are: `square` (for the FOMLS|SqPP), `random` (for the random-FOMLS|RPP) and `bagpipe` (for the ratio-FOMLS|RPP).

## Contributing

If you have any ideas, just open an issue and tell me what you think.

If you'd like to contribute, please fork the repository and make changes as you'd like. Pull requests are warmly welcome.