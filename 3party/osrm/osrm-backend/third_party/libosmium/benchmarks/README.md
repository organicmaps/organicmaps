
# Benchmarks

Benchmarks check the performance of different parts of Libosmium.

## Preparations

To run the benchmarks first make a directory for the data files somewhere
(outside the repository) and set the `DATA_DIR` environment variable:

    export DATA_DIR=benchmark_data
    mkdir $DATA_DIR

Then copy the OSM files you want to do the benchmarks with into this directory.
You can use the `download_data.sh` script to download a selection of OSM files
in different sizes, but you can use a different selection, too. The benchmarks
will use whatever files you have in the `DATA_DIR` directory.

The download script will start the data files names with a number in order of
the size of the file from smallest to largest. You can use the same convention
or use a different one. Benchmarks will be run on the files in alphabetical
order.

The files don't have to be in that directory, you can add soft links from that
directory to the real file locations if that suits you.

## Compiling the benchmarks

To build the benchmarks set the `BUILD_BENCHMARKS` option when configuring with
CMake and run the compilation by calling `make` (or whatever build tool you
are using).

## Running the benchmarks

Go to the build directory and run `benchmarks/run_benchmarks.sh`. You can also
run each benchmark on its own by calling the respective script in the
`benchmarks` directory.

Results of the benchmarks will be printed to stdout, you might want to redirect
them into a file.

