# Swapping Java functions on runtime with CafeSwap
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Overview
This project is insipred by SystematicSkid's [java JIT hooks](https://github.com/SystematicSkid/java-jit-hooks) project, it allows you to swap a function with another function on runtime, letting you, with reflection, add "hooks" anywhere in the code.

## Requirements
- A java application to hook
- Knowledge of the class and method name/signature to hook
- Address of `CompileBroker::compile_method`

## Dependencies
This project uses minhook to hook to the compile_method function.

## Building
To build the project, open the project in visual studio, change the Linker and the C/C++ options to include all the headers and libraries of Java.

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.