  Label1:
    JMPLBL Label2;
    JMPLBL Label2; // IGNORED
    JMPLBL Label2;
  Label2:
    JMPPC 2;
    JMPPC 100; // Ignored
    BREQ R64B_1, R64B_2, 2;
    BREQ R64B_1, R64B_2, 100; // Ignored
    BGT R64B_1, R64B_2, 2; 
    BGT R64B_1, R64B_2, 1; 
    BGET R64B_1, R64B_2, 2;
    BGET R64B_1, R64B_2, 1;
    BLT R64B_1, R64B_2, 2;
    BLT R64B_1, R64B_2, 1;
    BLET R64B_1, R64B_2, 2;
    BLET R64B_1, R64B_2, 1;
    ADD R64B_1, R64B_2, 255; 
    ADD R64B_1, R64B_1, 65280; 
    ADD R64B_1, R64B_1, 16711680; 
    ADD R64B_2, R64B_1, R64B_1; 
    SUB R64B_1, R64B_2, R64B_3; 
    SUB R64B_1, R64B_2, 100; 
    SHFL R64B_1, R64B_2;
    SHFL R64B_1, -100;
    SHFR R64B_1, R64B_2;
    SHFR R64B_1, -100;
    LDADR R64B_1, R64B_2;
    LDADR R64B_1, 02020;
    LDOFF R64B_1, R64B_2, R64B_3; 
    LDOFF R64B_1, R64B_2, 02020; 
    LDOFF R64B_1, 1010, 02020; 
    STADR R64B_1, R64B_2;
    STADR R64B_1, 02020;
    STOFF R64B_1, R64B_2, R64B_3; 
    STOFF R64B_1, R64B_2, 02020; 
    STOFF R64B_1, 1010, 02020; 
    COMMIT;
