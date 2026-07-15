# MIPS Assembler in C

A two-pass MIPS assembler written in C that translates MIPS assembly instructions into **32-bit hexadecimal machine code**. The assembler supports label resolution, multiple instruction formats, and generates output verified against the **MARS MIPS Simulator**.

---

## Features

- Two-pass assembler implementation
- Supports **R-Type**, **I-Type**, and **J-Type** instructions
- Automatic label detection and symbol table generation
- Branch offset calculation (`beq`)
- Jump target address calculation (`j`)
- Memory operand parsing (`offset($register)`)
- Outputs hexadecimal machine code with corresponding memory addresses
- Verified against the MARS MIPS Simulator

---

## Supported Instructions

### R-Type

- `add`
- `and`
- `or`
- `xor`
- `slt`

### I-Type

- `addi`
- `andi`
- `ori`
- `lw`
- `sw`
- `beq`

### J-Type

- `j`

---

## Project Structure

```
.
├── obil.c                 # Source code
├── in1.txt                # Test case 1
├── in2.txt
├── in3.txt
├── in4.txt
├── in5.txt
├── in6.txt
├── report/
│   └── Mips Report.pdf
├── README.md
├── LICENSE
└── .gitignore
```

---

## How It Works

The assembler follows the traditional **two-pass assembly** approach.

### Pass 1

- Reads the entire assembly program
- Detects labels
- Stores labels and their memory addresses in a symbol table

### Pass 2

- Reads each instruction
- Resolves labels
- Encodes instructions into 32-bit machine code
- Prints the final hexadecimal representation

---

## Example

### Input

```asm
start:
addi $t0, $zero, 5
addi $t1, $zero, 10
add $t2, $t0, $t1
j start
```

### Output

```
0x00400000 0x20080005 addi $t0,$zero,5
0x00400004 0x2009000A addi $t1,$zero,10
0x00400008 0x01095020 add $t2,$t0,$t1
0x0040000C 0x08100000 j start
```

---

## Compilation

Compile using GCC:

```bash
gcc obil.c -o assembler
```

Run:

```bash
./assembler in1.txt
```

---

## Verification

The generated machine code was verified against the **MARS MIPS Simulator**.

Testing includes:

- R-Type instructions
- I-Type instructions
- J-Type instructions
- Memory instructions
- Branch instructions
- Forward label references
- Backward jumps

All generated hexadecimal machine code matched the output produced by MARS.

---

## Concepts Demonstrated

- Compiler Construction
- Two-Pass Assembly
- Symbol Tables
- Instruction Encoding
- Bitwise Operations
- Register Mapping
- MIPS Architecture
- Machine Code Generation

---

## Technologies Used

- C
- GCC
- MARS MIPS Simulator
- Ubuntu Linux

---

## Future Improvements

- Support additional MIPS instructions (`sub`, `mult`, `div`, etc.)
- Better error handling
- Pseudo-instruction support
- Binary output generation
- ELF executable generation
- Command-line options
- Improved diagnostics

---

## Author

**Obil Abid**

Computer Science Student  
Forman Christian College (A Chartered University)

GitHub: https://github.com/OBIL2

---

## License

This project is licensed under the MIT License.
