# Program tester

## Requirements
- Bash 
- Valgrind

## Usage
- Script needs to be in folder with inputs,outputs files
- Script tests the .c files, not the compiled program 
- Input and Output files needs to end with `_in.txt` `_out.txt` (ex. `0000_in.txt`, `0000_out.txt`)
- Script compiles the .c file and saves it as `pgr_test.exe` 

## Compilation
> The script compiles the program with -Wall -pedantic and -o2 optimization flags
+ If the compiler returns any warning, the script will not test the program even if the program can be compiled  
  - Progtest evaluates program that has warnings in compilation as fail

## Authors
Created by Robin Mitan
