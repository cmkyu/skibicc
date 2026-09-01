# skibicc

**A Work-in-Progress C Compiler**

Skibicc is a hobbyist C compiler written in C. It is currently under active development, with a focus on education and experimentation. At this stage, the lexer is nearly fully featured, and the compiler can parse and generate code for a very limited subset of C.

## What Works

- **Lexer:** The lexer is almost fully feature-complete. It can correctly tokenize a large portion of the C language, including:
  - Keywords and identifiers.
  - Integer, floating-point, and character constants (decimal, octal, and hexadecimal).
  - String literals (UTF-8, UTF-16, and UTF-32).
  - All C punctuators.
  - Support for most escape sequences.
  - Pretty error and warning messages with colored highlights and squiggly lines.

- **Parser:** The parser can handle a very limited subset of C expressions and statements:
  - Unary, binary, logical, and relational operators (with precedence and associativity).
  - Assignment expressions.
  - Return statements.
  - A very basic main function definition syntax.

- **Code Generation:** A simple, not-yet-optimized x86_64 code generator is in place. It can generate assembly code from the compiler's intermediate representation (IR) for basic arithmetic, logical, and relational operations.

- **Intermediate Representation (IR):** Skibicc uses its own IR to represent the program before generating machine code.

- **Testing:** The project uses the [Unity](https://github.com/ThrowTheSwitch/Unity) test framework. There are extensive unit tests for the lexer, as well as tests for the parser, IR, and code generator.

## What Doesn't Work (Yet)

This is a very early-stage compiler. The following are not supported or are severely limited:

- **Types:** Only `int` and `void` are supported. No arrays, pointers, structs, unions, or other complex types.
- **Statements:** Only `return` and basic expression statements are supported. No `if`, `for`, `while`, `switch`, etc.
- **Functions:** Only a single, parameterless `main` function is supported. No user-defined functions.
- **Preprocessor:** The compiler currently relies on the system's C preprocessor (`cpp`) for preprocessing (e.g., `#include`).
- **Linking:** The compiler uses the system's linker (`ld`) to produce an executable.
- **Error Reporting:** No robust semantic analysis is performed yet.

## Platform Support

**Currently, skibicc only works on Arch Linux (or derivatives) out of the box.** This is because the paths to the C runtime object files (e.g., `crt1.o`, `crti.o`, `crtbeginS.o`, `crtendS.o`, `crtn.o`) are hardcoded in the linking command inside `skibicc.c`.

If you are using a different Linux distribution (or macOS), you will need to edit the `emit_code()` function in `skibicc.c` and adjust the `ld` command line to point to the correct locations of these object files on your system.

To find the correct paths on your system, you can use:

```bash
gcc -print-file-name=crt1.o
gcc -print-file-name=crti.o
gcc -print-file-name=crtn.o
gcc -print-file-name=crtbeginS.o
gcc -print-file-name=crtendS.o
```

Also, make sure the dynamic linker path (`-dynamic-linker /lib64/ld-linux-x86-64.so.2`) matches your system's `ld-linux.so`.

The compiler is only tested on x86_64 Linux.

## Build

### Prerequisites

- GCC (or Clang)
- GNU Make
- The `cpp` preprocessor (usually provided by `gcc` or `clang`)
- The `as` assembler (from GNU Binutils)
- The `ld` linker (from GNU Binutils)
- The `gcc` runtime libraries (for linking)

### Compiling

To build the compiler:

```bash
make debug   # Builds with debug symbols
```

or

```bash
make release # Builds with optimizations (-O3)
```

The resulting executable, `skibicc`, will be placed in the `bin/` directory.

## Usage

Currently, skibicc can only compile a very minimal C program.

```bash
./bin/skibicc <path-to-c-file>
```

The compiler will:

1.  Run the preprocessor.
2.  Lex, parse, and generate IR.
3.  Generate x86_64 assembly.
4.  Assemble and link the resulting object code.

The output executable will be placed in the same directory as the source file, with a `.out` extension. For example, compiling `myprogram.c` will produce `myprogram.out`.

### Command-Line Options

Skibicc supports a few command-line flags for development and debugging:

- `--lex`: Only run the lexer.
- `--parse`: Run the lexer and parser.

These options are useful for understanding how the compiler processes a C file.

## Testing

Skibicc uses Unity for unit testing. To run the test suite:

```bash
make test
```

This will compile and run all the tests. A summary of passed and failed tests will be displayed.

## Project Structure

The project is organized into the following key components:

- **Lexer (`lexer.c`, `lexer.h`):** Scans the source code and produces a stream of tokens.
- **Parser (`parser.c`, `parser.h`):** Parses the token stream and builds an Abstract Syntax Tree (AST).
- **IR (`ir.c`, `ir.h`):** Generates an Intermediate Representation from the AST.
- **Code Generator (`codegen.c`, `codegen.h`):** Translates the IR into x86-64 assembly.
- **Utilities (`array`, `hashmap`, `list`, `strings`, `unicode`, `errors`):** Provides various helper data structures and functions.

## Contributing

This is a personal project, but contributions, suggestions, and bug reports are welcome! Feel free to open an issue or a pull request.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details (if one exists).

## Acknowledgements

This project has drawn inspiration from several sources, including:
- The [chibicc](https://github.com/rui314/chibicc) compiler by Rui Ueyama.
- *Writing a C Compiler* by Sandler Nora.
- Various C compiler tutorials and blog posts.

## Future Work

The next steps for this project include:

- **Semantic Analysis:** Adding type checking and a symbol table.
- **Control Flow:** Implementing `if` statements, loops, and other control flow constructs.
- **Functions:** Supporting user-defined functions and function calls.
- **Pointers and Arrays:** Adding support for pointer arithmetic and arrays.
- **More Types:** Supporting `char`, `short`, `long`, `float`, `double`, etc.
- **Optimizations:** Implementing basic optimizations in the IR and code generator.
- **Self-Hosting:** Eventually, the goal is for skibicc to be able to compile itself.
