# MiniC Compiler Project  
## From MiniC Source Code to MIPS Assembly

## Description

This project consists in the complete implementation of a **compiler for the MiniC language**, a deliberately restricted subset of the C language designed for educational purposes.

MiniC enforces **strong typing**, does not support pointers or arrays, and limits language constructs in order to focus on the **fundamental principles of compilation**.

The developed compiler translates a MiniC program into **MIPS assembly code**, following a classical multi-pass compilation architecture:
- lexical analysis
- syntax analysis with Abstract Syntax Tree (AST) construction
- contextual and semantic verification
- MIPS code generation

The project was developed incrementally, with systematic testing at each stage.

---

## Compiler Architecture

The compiler is organized into independent modules, each corresponding to a specific compilation phase.  
This modular design simplifies reasoning, debugging, and validation.

Main components:

- `lexico.l` – Lexical analysis (Lex)
- `grammar.y` – Syntax analysis and AST construction (Yacc)
- `defs.h` – AST node definitions
- `passe_1.c` – Contextual verification and type checking
- `passe_2.c` – MIPS code generation
- `scripts/` – Automated test scripts
- `Tests/` – Syntax, semantic, and code generation test cases

The global compilation flow is:

1. Source code → tokens  
2. Tokens → Abstract Syntax Tree (AST)  
3. AST → semantic enrichment (types, scopes, offsets)  
4. Enriched AST → MIPS assembly code  

---

## Lexical Analysis

The lexical analyzer recognizes all MiniC language tokens, including:

- Keywords (`int`, `bool`, `void`, `if`, `while`, `for`, `do`, `print`, etc.)
- Identifiers
- Literals:
  - integers
  - booleans
  - strings

Special care was taken to correctly handle:
- string literals (value preservation for code generation)
- line number tracking (`yylineno`)
- lexical error reporting with precise line information

Comments and whitespace are ignored.

---

## Syntax Analysis and AST Construction

The syntax analysis is implemented using Yacc and follows the official MiniC grammar, including:

- global and local declarations
- nested blocks
- control structures (`if`, `while`, `do-while`, `for`)
- expressions (arithmetic, logical, bitwise)
- assignments as expressions

Each grammar rule constructs nodes of an **Abstract Syntax Tree (AST)** using a generic node constructor.  
At this stage, the AST represents only the grammatical structure of the program.

---

## AST Visualization and Validation

To validate the correctness of the AST construction, tree dumps are generated in `.dot` format and converted to PDF using Graphviz.

This visualization step allows verification of:
- block nesting
- declaration hierarchy
- expression structure
- instruction ordering

Two AST versions are produced:
- after syntax analysis
- after contextual verification (semantic enrichment)

---

## Pass 1 – Contextual and Semantic Verification

The first pass is the semantic core of the compiler. Its responsibilities include:

- identifier resolution
- scope management
- strong type checking
- enforcement of MiniC semantic rules

### Environments and Scopes

The compiler uses:
- one global context
- a stack of local contexts for nested blocks

This mechanism prevents duplicate declarations in the same scope while allowing shadowing in nested scopes.

### Identifier Resolution

Each identifier usage is linked to its declaration node, allowing retrieval of:
- type
- memory offset
- global or local status

Any use of an undeclared variable is immediately reported as an error.

### Type Checking

Strict typing rules are enforced:
- arithmetic operations: `int → int`
- comparisons: `int → bool`
- logical operations: `bool → bool`
- bitwise and shift operations: `int → int`
- assignments: identical types only

No implicit type conversions are allowed.

### Special Rules

- `main` must be named `main` and return `void`
- conditions in control structures must be boolean
- string literals are prepared for code generation without type checking

---

## Pass 2 – MIPS Code Generation

The second pass translates the enriched AST into **MIPS assembly code**.

Code generation is performed through a recursive traversal of the AST, using provided utility functions for:
- temporary register management
- label generation
- system calls (`print_int`, `print_string`, `exit`)

### Expression and Control Flow Translation

- expressions are evaluated using temporary registers
- assignments generate appropriate `lw` and `sw` instructions
- control structures are translated using conditional branches and generated labels

### Print Instruction

The `print` instruction supports:
- string literals → `print_string` syscall
- expressions → `print_int` syscall

This distinction relies on semantic information computed during pass 1.

---

## Generated Assembly Output

The final output is a MIPS assembly file named `out.s`, structured into:

### `.data` Section
Contains global variable declarations:
- labels
- `.word` directives
- initial values

### `.text` Section
Contains executable code:
- `main` label (entry point)
- stack initialization
- translated instructions
- program termination syscall

Each instruction directly corresponds to a node in the enriched AST.

---

## Testing and Validation

Tests were developed incrementally and are organized into:
- syntax tests
- semantic verification tests
- code generation tests

Each category includes valid (OK) and invalid (KO) cases.

A `run_tests.sh` script executes the full test suite and validates compiler behavior under different modes.

---

## Implemented Features and Limitations

### Implemented
- complete lexical and syntax analysis
- strong type checking
- scope and identifier management
- functional MIPS code generation
- support for main MiniC control structures

### Limitations
- no code optimizations
- limited static detection of runtime errors
- dependence on a MIPS emulator for execution

These limitations are consistent with the pedagogical objectives of the project.

---

## Conclusion

This project provides a complete and functional MiniC compiler, covering all major stages of compilation from lexical analysis to assembly code generation.

The incremental development approach and systematic testing strategy resulted in a robust and specification-compliant compiler, forming a solid foundation for further extensions or optimizations.

---

## Author and Supervisor

- **Author:** Papa Talla Dioum  
- **Supervisor:** Quentin Meunier
