#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================
   Obil Nathaniel - 271048001
   MIPS Assembler written in C
   
   What this program does:
   - Reads a MIPS assembly text file from the command line
   - Prints the original assembly program to the console
   - Performs two passes over the input:
       PASS 1: Scans for labels and records their addresses
       PASS 2: Translates each instruction into 32-bit machine code
   - Outputs three columns: address, hex machine code, original instruction
   - Supports 12 MIPS instructions:
     add, and, or, xor, slt (R-type)
     addi, andi, ori, lw, sw, beq (I-type)
     j (J-type)
   - Base address starts at 0x00400000, increments by 4 each instruction
   ============================================================ */

#define MAX_LINES     1000
#define MAX_LINE_LEN  256
#define BASE_ADDRESS  0x00400000

/* Structure to store a label and its corresponding memory address */
typedef struct {
    char name[64];
    int  address;
} Label;

Label labelTable[200]; /* Table holding all labels found in the program */
int   labelCount = 0;  /* Number of labels found so far                 */

/* ------------------------------------------------------------
   getRegNum - converts a register name to its numeric value
   Accepts both named registers ($t0, $sp, etc.)
   and numeric registers ($8, $29, etc.)
   ------------------------------------------------------------ */
int getRegNum(char *reg) {
    char *r = reg;
    if (r[0] == '$') r++; /* strip the leading dollar sign */

    /* named register mappings */
    if (strcmp(r, "zero") == 0) return 0;
    if (strcmp(r, "at")   == 0) return 1;
    if (strcmp(r, "v0")   == 0) return 2;
    if (strcmp(r, "v1")   == 0) return 3;
    if (strcmp(r, "a0")   == 0) return 4;
    if (strcmp(r, "a1")   == 0) return 5;
    if (strcmp(r, "a2")   == 0) return 6;
    if (strcmp(r, "a3")   == 0) return 7;
    if (strcmp(r, "t0")   == 0) return 8;
    if (strcmp(r, "t1")   == 0) return 9;
    if (strcmp(r, "t2")   == 0) return 10;
    if (strcmp(r, "t3")   == 0) return 11;
    if (strcmp(r, "t4")   == 0) return 12;
    if (strcmp(r, "t5")   == 0) return 13;
    if (strcmp(r, "t6")   == 0) return 14;
    if (strcmp(r, "t7")   == 0) return 15;
    if (strcmp(r, "s0")   == 0) return 16;
    if (strcmp(r, "s1")   == 0) return 17;
    if (strcmp(r, "s2")   == 0) return 18;
    if (strcmp(r, "s3")   == 0) return 19;
    if (strcmp(r, "s4")   == 0) return 20;
    if (strcmp(r, "s5")   == 0) return 21;
    if (strcmp(r, "s6")   == 0) return 22;
    if (strcmp(r, "s7")   == 0) return 23;
    if (strcmp(r, "t8")   == 0) return 24;
    if (strcmp(r, "t9")   == 0) return 25;
    if (strcmp(r, "k0")   == 0) return 26;
    if (strcmp(r, "k1")   == 0) return 27;
    if (strcmp(r, "gp")   == 0) return 28;
    if (strcmp(r, "sp")   == 0) return 29;
    if (strcmp(r, "fp")   == 0) return 30;
    if (strcmp(r, "ra")   == 0) return 31;

    /* if it starts with a digit, treat it as a plain number e.g. $9 */
    if (isdigit((unsigned char)r[0])) return atoi(r);

    return -1; /* unknown register */
}

/* ------------------------------------------------------------
   trim - removes leading and trailing whitespace and newlines
   from a string, modifying it in place
   ------------------------------------------------------------ */
void trim(char *s) {
    /* remove leading whitespace */
    int i = 0;
    while (s[i] == ' ' || s[i] == '\t') i++;
    if (i > 0) memmove(s, s + i, strlen(s) - i + 1);

    /* remove trailing whitespace and newline characters */
    int len = strlen(s);
    while (len > 0 && (s[len-1] == ' '  || s[len-1] == '\t' ||
                        s[len-1] == '\n' || s[len-1] == '\r'))
        s[--len] = '\0';
}

/* ------------------------------------------------------------
   isLabel - returns 1 if the line is a label definition
   A label must end with ':' and contain no spaces inside
   e.g. "start:" or "loop:" are labels
   ------------------------------------------------------------ */
int isLabel(char *line) {
    int len = strlen(line);
    if (len < 2)            return 0; // too short to be a label  
    if (line[len-1] != ':') return 0; // must end with colon      
    /* if there is a space inside, it is not a label */
    for (int i = 0; i < len-1; i++)
        if (line[i] == ' ' || line[i] == '\t') return 0;
    return 1;
}

/* ------------------------------------------------------------
   getLabelAddress - looks up a label name in the label table
   and returns its memory address
   Returns -1 if the label is not found
   ------------------------------------------------------------ */
int getLabelAddress(char *name) {
    /* strip trailing colon if accidentally passed in */
    char clean[64];
    strncpy(clean, name, 63);
    clean[63] = '\0';
    int l = strlen(clean);
    if (l > 0 && clean[l-1] == ':') clean[l-1] = '\0';

    for (int i = 0; i < labelCount; i++)
        if (strcmp(labelTable[i].name, clean) == 0)
            return labelTable[i].address;
    return -1; // label not found 
}

/* ------------------------------------------------------------
   parseMemOperand - parses a memory operand like 16($t0) or ($t2)
   Extracts the offset (integer) and the base register number
   Returns the register number and sets *offset via pointer
   ------------------------------------------------------------ */
int parseMemOperand(char *operand, int *offset) {
    char temp[128];
    strcpy(temp, operand);
    trim(temp);

    char *paren = strchr(temp, '(');
    if (!paren) {
        /* no parenthesis found - treat whole thing as register, offset = 0 */
        *offset = 0;
        return getRegNum(temp);
    }

    *paren = '\0';           // split at the opening parenthesis 
    char *offsetStr = temp;  // everything before '(' is the offset 
    char *regPart   = paren + 1; // everything after '(' is the register 

    /* remove the closing parenthesis */
    char *closeParen = strchr(regPart, ')');
    if (closeParen) *closeParen = '\0';

    trim(offsetStr);
    trim(regPart);

    /* if nothing before the paren, offset is 0 e.g. ($t2) */
    *offset = (strlen(offsetStr) == 0) ? 0 : atoi(offsetStr);
    return getRegNum(regPart);
}

/* ============================================================
   main - entry point
   Accepts the input filename as a command line argument
   ============================================================ */
int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage: %s <inputFile>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        printf("Error: Cannot open file '%s'\n", argv[1]);
        return 1;
    }

    char lines[MAX_LINES][MAX_LINE_LEN];
    int  lineCount = 0;

    /* read every line from the file into the lines array */
    while (lineCount < MAX_LINES && fgets(lines[lineCount], MAX_LINE_LEN, fp)) {
        trim(lines[lineCount]);
        lineCount++;
    }
    fclose(fp);

    /* --------------------------------------------------------
       Print the original assembly program as-is to the console
       -------------------------------------------------------- */
    printf("Assembly language program:\n");
    for (int i = 0; i < lineCount; i++)
        if (strlen(lines[i]) > 0)
            printf("%s\n", lines[i]);
    printf("\n");

    /* --------------------------------------------------------
       PASS 1 - Build the label table
       Walk through every line, if it is a label record its
       name and the address it corresponds to (current addr).
       Non-label lines advance the address counter by 4 bytes.
       -------------------------------------------------------- */
    int addr = BASE_ADDRESS;
    for (int i = 0; i < lineCount; i++) {
        char tmp[MAX_LINE_LEN];
        strcpy(tmp, lines[i]);
        if (!strlen(tmp)) continue; // skip blank lines 

        if (isLabel(tmp)) {
            tmp[strlen(tmp)-1] = '\0'; // remove the colon 
            trim(tmp);
            strcpy(labelTable[labelCount].name, tmp);
            labelTable[labelCount].address = addr;
            labelCount++;
        
        } else {
            addr += 4; /* each instruction occupies 4 bytes */
        }
    }

    /* --------------------------------------------------------
       PASS 2 - Translate each instruction to machine code
       For each non-label line, identify the instruction type,
       extract operands and build the 32-bit binary encoding
       then print address, hex machine code and original line
       -------------------------------------------------------- */
    printf("Machine Code:\n");
    addr = BASE_ADDRESS;

    for (int i = 0; i < lineCount; i++) {
        char line[MAX_LINE_LEN];
        strcpy(line, lines[i]);
        if (!strlen(line)) continue; // skip blank lines  
        if (isLabel(line))  continue; // skip label lines 

        char original[MAX_LINE_LEN];
        strcpy(original, line); 

        char mnemonic[32], op1[64], op2[64], op3[64];
        mnemonic[0]=op1[0]=op2[0]=op3[0]='\0';

        /* extract mnemonic (first token) */
        char tmp[MAX_LINE_LEN];
        strcpy(tmp, line);
        sscanf(tmp, "%s", mnemonic);

        /* find everything after the mnemonic */
        char *rest = tmp + strlen(mnemonic);
        while (*rest == ' ' || *rest == '\t') rest++;

        /* split the remaining operands by comma */
        char *tok = strtok(rest, ",");
        char *operands[3] = {op1, op2, op3};
        int oc = 0;
        while (tok && oc < 3) {
            // trim each operand individually 
            while (*tok == ' ' || *tok == '\t') tok++;
            int tl = strlen(tok);
            while (tl > 0 && (tok[tl-1]==' '||tok[tl-1]=='\t')) tok[--tl]='\0';
            strcpy(operands[oc++], tok);
            tok = strtok(NULL, ",");
        }

        unsigned int machineCode = 0;

        /* ====================================================
           R-TYPE INSTRUCTIONS
           Format: opcode(6) rs(5) rt(5) rd(5) shamt(5) funct(6)
           opcode is always 0 for R-type
           funct field differentiates between R-type instructions
           ==================================================== */

        if (strcmp(mnemonic,"add")==0) {
            /* add $rd, $rs, $rt   funct = 0x20 */
            int rd=getRegNum(op1), rs=getRegNum(op2), rt=getRegNum(op3);
            machineCode=(0<<26)|(rs<<21)|(rt<<16)|(rd<<11)|(0<<6)|0x20;
        }
        else if (strcmp(mnemonic,"and")==0) {
            /* and $rd, $rs, $rt   funct = 0x24 */
            int rd=getRegNum(op1), rs=getRegNum(op2), rt=getRegNum(op3);
            machineCode=(0<<26)|(rs<<21)|(rt<<16)|(rd<<11)|(0<<6)|0x24;
        }
        else if (strcmp(mnemonic,"or")==0) {
            /* or $rd, $rs, $rt   funct = 0x25 */
            int rd=getRegNum(op1), rs=getRegNum(op2), rt=getRegNum(op3);
            machineCode=(0<<26)|(rs<<21)|(rt<<16)|(rd<<11)|(0<<6)|0x25;
        }
        else if (strcmp(mnemonic,"xor")==0) {
            /* xor $rd, $rs, $rt   funct = 0x26 */
            int rd=getRegNum(op1), rs=getRegNum(op2), rt=getRegNum(op3);
            machineCode=(0<<26)|(rs<<21)|(rt<<16)|(rd<<11)|(0<<6)|0x26;
        }
        else if (strcmp(mnemonic,"slt")==0) {
            /* slt $rd, $rs, $rt   funct = 0x2A   */
            int rd=getRegNum(op1), rs=getRegNum(op2), rt=getRegNum(op3);
            machineCode=(0<<26)|(rs<<21)|(rt<<16)|(rd<<11)|(0<<6)|0x2A;
        }

        /* ====================================================
           I-TYPE INSTRUCTIONS
           Format: opcode(6) rs(5) rt(5) immediate(16)
           ==================================================== */

        else if (strcmp(mnemonic,"addi")==0) {
            /* addi $rt, $rs, imm   opcode = 0x08
               */
            int rt=getRegNum(op1), rs=getRegNum(op2), imm=atoi(op3);
            machineCode=(0x08<<26)|(rs<<21)|(rt<<16)|((unsigned short)(short)imm);
        }
        else if (strcmp(mnemonic,"andi")==0) {
            /* andi $rt, $rs, imm   opcode = 0x0C
              */
            int rt=getRegNum(op1), rs=getRegNum(op2), imm=atoi(op3);
            machineCode=(0x0C<<26)|(rs<<21)|(rt<<16)|((unsigned short)(short)imm);
        }
        else if (strcmp(mnemonic,"ori")==0) {
            /* ori $rt, $rs, imm   opcode = 0x0D*/
            int rt=getRegNum(op1), rs=getRegNum(op2), imm=atoi(op3);
            machineCode=(0x0D<<26)|(rs<<21)|(rt<<16)|((unsigned short)(short)imm);
        }
        else if (strcmp(mnemonic,"lw")==0) {
            /* lw $rt, offset($rs)   opcode = 0x23 */
            int rt=getRegNum(op1);
            int offset=0;
            int rs=parseMemOperand(op2,&offset);
            machineCode=(0x23<<26)|(rs<<21)|(rt<<16)|((unsigned short)(short)offset);
        }
        else if (strcmp(mnemonic,"sw")==0) {
            /* sw $rt, offset($rs)   opcode = 0x2B*/
            int rt=getRegNum(op1);
            int offset=0;
            int rs=parseMemOperand(op2,&offset);
            machineCode=(0x2B<<26)|(rs<<21)|(rt<<16)|((unsigned short)(short)offset);
        }
        else if (strcmp(mnemonic,"beq")==0) {
            /* beq $rs, $rt, label   opcode = 0x04
               branches to label if rs == rt
              */
            int rs=getRegNum(op1), rt=getRegNum(op2);
            int labelAddr=getLabelAddress(op3);
            int offset=(labelAddr!=-1)?(labelAddr-(addr+4))/4:0;
            machineCode=(0x04<<26)|(rs<<21)|(rt<<16)|((unsigned short)(short)offset);
        }

        /* ====================================================
           J-TYPE INSTRUCTIONS
           Format: opcode(6) target(26)
           target = label_address >> 2  (word address, lower 26 bits)
           ==================================================== */

        else if (strcmp(mnemonic,"j")==0) {
            /* j label   opcode = 0x02
               unconditional jump to label address */
            int labelAddr=getLabelAddress(op1);
            unsigned int target=(labelAddr!=-1)?((unsigned int)labelAddr>>2)&0x03FFFFFF:0;
            machineCode=(0x02<<26)|target;
        }
        else {
            /* instruction not recognised - skip and warn */
            printf("Unknown instruction: %s\n", mnemonic);
            addr+=4;
            continue;
        }

        /* print the three columns: address | machine code | original instruction */
        printf("0x%08X 0x%08X %s\n", addr, machineCode, original);
        addr+=4;
    }
    return 0;
}
