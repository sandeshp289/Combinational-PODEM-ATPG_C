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


Notes:


c1.lev :  Fig. 3.22, Pg. 133

c2.lev:   Fig. 4.6,  Pg. 168

c15.lev:  Fig. 4.35, Pg. 201

c15a.lev: Fig. 4.11, Pg. 175



c1:   21/22

c2:   16/21

c15:  19/26

c15a: 6/12

c17:  38/38


c499: 1118/1126         Fault Simulate: 1112/1126

c432: 544/738           Fault SImulate: 699/738

c880: 1572/1578         Fault Simulate: 1574/1578
