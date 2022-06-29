# Original Version by daniel4lee
<https://github.com/daniel4lee/MIPS-Pipeline-Simulator>

# MIPS-Pipeline-Simulator

This program implement the simple MIPS pipeline, including the following instructions `"lw"`, `"sw"`, `"add"`, `"addi"`, `"sub"`, `"and"`, `"andi"`, `"or"`, `"slt"`, `"bne"`.
In addition to the instructions, it also support detecting and dealing `"data hazard"`, `"hazard with load"`, `"branch hazard"`.  Data forwarding has been removed in this version (data hazards cause delays).

Note that in this version, there are some issues involving `"bne"` and tracking the cycles that instructions get written back on.  Programs using `"bne"` will not always be simulated perfectly.
Also note that `"sub"` has been coded to take 3 cycles in the execute stage of the pipeline, as an example.
## Initialize

There are ten registers, and their initial value is as below:
![list_register](https://i.imgur.com/sryYf15.png)

The memory initial value is as below:  
![list_memory](https://i.imgur.com/NeRcky4.png)

p.s. instruction memory is start from 0.

## Input

The program will read the the `input.txt` file which in the same root, and the content of the file is the 32 bit machine codes.
The  format should like below.

```
10001101000000010000000000000011  
00000000000000100001100000100000  
00010000000000100000000000000110
```
The correspounding MIPS instructions is:
```shell
lw $1, 0x03($8)  
add $3, $0, $2  
beq $0, $2, 0x06 #(branch PC+4+4×6)   
```

## Output

Originally, after executing the program, it would simulate every pipeline register's value on each clock cycle and save the record as a `.txt` file named `Result`.  This output has been replaced (the code is simply commented out; the register values are still simulated).  Now it outputs the following data for each instruction: `Cycle Fetched,Cycle Written Back,OP Code,Destination Register,Source Register1,Source Register2,isStore,isLoad,isBranch,Total Execution Time (Cycle Written Back - Cycle Fetched + 1)` in that format, with -1s denoting a value is not applicable for the instruction (0 is false, 1 is true). 

### The content description (old version): 
![format](https://i.imgur.com/HWRKnln.png)