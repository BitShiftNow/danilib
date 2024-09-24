# danilib

A collection of my personal single-file C libraries I use to develop my applications.
The only dependency for most of these files is the base header which contains all base types and helper macros.
If a file has any other dependencies they will be listed in the comment section at the beginning of the file.


## Info

Build Platform: `Windows 11 Home 64-bit - 23H2 - 22631.4169`

Compiler: `Microsoft (R) C/C++ Optimizing Compiler Version 19.38.33135 for x64`


## How to use

Select the library you would like to use and copy it to your projects source code directory. Additional configuration can be done by either editing the file or by supplying known definitions before including. See the comment section at the beginning of each library file to find out more about all the different available configurations.


## Libraries

The following libraries are available:

| Library | Description |
| ------------- | ------------- |
| dani_base.h | Contains base types and helper macros that are used by all other library files. |
| dani_profiler.h | Contains functionality to collect profiling information based on rdtsc. |


## License

Each library has licence information included at the end of the file. It is based on the zlib license and can also be found [here](LICENSE).
