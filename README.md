# nand2tetris-vmtranslator
VM Translator for Nand2Tetris course, written in C++

## Requirements
gcc

## Installation
```
git clone https://github.com/Arcademic/nand2tetris-vmtranslator.git
cd nand2tetris-vmtranslator
make
```

## Usage
For translating a single VM file:
```
cd target
./vmtranslator path/to/file.vm
```
For translating multiple VM files:
```
cd target
./vmtranslator path/to/dir
```
Bootstrap code is only generated when providing a directory
