# Combinational-PODEM-ATPG

Description:

    PODEM ATPG for Combinational circuits in C/C++

Input Files:

    podem.c                          : C/C++ code

    c17.lev                          : Input circuit file (for e.g. c17) in lev format

Output Files:

    c17.vec                          : Output deterministic vector file (for e.g. c17)

    c17.det                          : Detected Faults

    c17.undet                        : Undetected Faults
    
Compilation:

    g++ podem.c
    
Execution:

    ./a.out c17
