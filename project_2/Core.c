#include "Core.h"

Core *initCore(Instruction_Memory *i_mem)
{
    Core *core = (Core *)malloc(sizeof(Core));
    core->clk = 0;
    core->PC = 0;
    core->instr_mem = i_mem;
    core->tick = tickFunc;

    // FIXME, initialize register file here.
    //Breaks with numbers > 1 byte
    core->data_mem[0] = 16;
    core->data_mem[8*1] = 128;
    core->data_mem[8*2] = 8;
    core->data_mem[8*3] = 4;

    // FIXME, initialize data memory here.
    core->reg_file[25] = 4;
    core->reg_file[10] = 4;
    core->reg_file[22] = 1;

    // for(int i = 0; i < 4; i++) {
    //     printf("%x\n",core->data_mem[8*i]);
    // }

    return core;
}

// FIXME, implement this function
bool tickFunc(Core *core)
{
    printf("PC: %d\n",core->PC);
    unsigned rd  = 0;
    unsigned rs1 = 0;
    unsigned rs2 = 0;

    // Steps may include
    // (Step 1) Reading instruction from instruction memory
    //printf("Step 1\n");
    unsigned instruction = core->instr_mem->instructions[core->PC / 4].instruction;
    
    // (Step 2) Instruction decode / register file read
    //printf("Step 2\n");
    ControlSignals *signals = (ControlSignals *)malloc(sizeof(ControlSignals));
    Signal opcode = (instruction & 127);
    printf("Opcode: %d\n",opcode);
    ControlUnit(opcode, signals);


    rd = (instruction & 3968) >> 7;
    printf("rd = %d\n",rd);
    rs1 = (instruction & 1015808) >> 15;
    printf("rs1 = %d\n",rs1);
    rs2 = (instruction & 32505856) >> 20;
    printf("rs2 = %d\n",rs2);

    Signal readReg1 = core->reg_file[rs1];
    printf("Register %d contents: %d\n",rs1, core->reg_file[rs1]);
    Signal readReg2 = core->reg_file[rs2];
    printf("Register %d contents: %d\n",rs2, core->reg_file[rs2]);
    
    Signal immediate = ImmeGen(instruction);
    printf("immediate: %d\n", immediate);

    // (Step 3) Execute / address calculation
    //printf("Step 3\n");
    Signal ALU_ctrl_signal;
    Signal Funct7 = (instruction & 4261412864) >> 25;
    Signal Funct3 = (instruction & 28672) >> 12;
    printf("Funct3: %d\n",Funct3);
    Signal ALUinput2 = MUX(signals->ALUSrc, readReg2, immediate);

    ALU_ctrl_signal = ALUControlUnit(signals->ALUOp, Funct7, Funct3);
    
    printf("ALU Control Signal: %d\n",ALU_ctrl_signal);
    Signal zero;
    Signal ALU_result;
    ALU(readReg1, ALUinput2, ALU_ctrl_signal, &ALU_result, &zero);
    printf("ALU RESULT: %d\n",ALU_result);

    Addr nextPC = MUX((signals->Branch && !zero), 
                    (core->PC + 4), 
                    (core->PC + ShiftLeft1(immediate)));
    // (Step 4) Memory access
    //printf("Step 4\n");
    Signal readMem;
    //Probably broken.
    if (signals->MemWrite == 1)
    {
        printf("Storing %d in memory location %d\n",readReg2, ALU_result*4);
        core->data_mem[ALU_result * 4] = readReg2;
    }
    if (signals->MemRead == 1)
    {
        readMem = core->data_mem[ALU_result];
    }

    // (Step 5) Write back
    //printf("Step 5\n");
    if (signals->RegWrite == 1)
    {
        if (signals->MemtoReg == 1)
        {
            printf("Storing %d in register %d\n",readMem, rd);
            core->reg_file[rd] = readMem;
        }
        else
        {
            printf("Storing %d in register %d\n",ALU_result, rd);
            core->reg_file[rd] = ALU_result;
        }
    }
    

    // (Step N) Increment PC. FIXME, is it correct to always increment PC by 4?!
    //GET OPCODE
    //Compare Branch signal with zero (output from ALU)
    core->PC = nextPC;

    ++core->clk;
    free(signals);
    // Are we reaching the final instruction?
    printf("x9 = %d, x11 = %d, PC going to %d\n\n",
core->reg_file[9],core->reg_file[11], core->PC);
    
    if (core->PC > core->instr_mem->last->addr)
    {
        return false;
    }
    return true;
}

// FIXME (1). Control Unit. Refer to Figure 4.18.
void ControlUnit(Signal input,
                 ControlSignals *signals)
{
    switch(input) {
        case 51: // add
            signals->ALUSrc = 0;
            signals->MemtoReg = 0;
            signals->RegWrite = 1;
            signals->MemRead = 0;
            signals->MemWrite = 0;
            signals->Branch = 0;
            signals->ALUOp = 2;
            break;
        case 3: //  ld
            signals->ALUSrc = 1;
            signals->MemtoReg = 1;
            signals->RegWrite = 1;
            signals->MemRead = 1;
            signals->MemWrite = 0;
            signals->Branch = 0;
            signals->ALUOp = 0;
            break;
        case 19: // slli / addi
            signals->ALUSrc = 1;
            signals->MemtoReg = 0;
            signals->RegWrite = 1;
            signals->MemRead =  0;
            signals->MemWrite = 0;
            signals->Branch = 0;
            signals->ALUOp = 0;
            break;
        case 99: // bne
            signals->ALUSrc = 0;
            signals->MemtoReg = 0;
            signals->RegWrite = 0;
            signals->MemRead =  0;
            signals->MemWrite = 0;
            signals->Branch = 1;
            signals->ALUOp = 1;
            break;
    }
}

// FIXME (2). ALU Control Unit. Refer to Figure 4.12.
Signal ALUControlUnit(Signal ALUOp,
                      Signal Funct7,
                      Signal Funct3)
{
    //printf("ALU Control Unit\n");
    // For add
    printf("ALUOp: %d\n",ALUOp);
    if (ALUOp == 2 && Funct7 == 0 && Funct3 == 0)
    {
        return 2;
    }
    else if (ALUOp == 0 && Funct3 == 0)
    {
        return 2;
    }
    else if (ALUOp == 0 && Funct3 == 3)
    {
        return 2;
    }
    else if (ALUOp == 0 && Funct3 == 1)
    {
        return 4;
    }
    else if (ALUOp == 1)
    {
        return 3;
    }
}

// FIXME (3). Imme. Generator
Signal ImmeGen(Signal input)
{
    int16_t immediate = 0;
    Signal opcode = (input & 127);

    if (opcode == 3 || opcode == 19) 
    {
        immediate = (uint16_t)((input & 4293918720) >> 20);
        unsigned signbit = (immediate & 2048) >> 11;
        immediate |= signbit << 12;
        immediate |= signbit << 13;
        immediate |= signbit << 14;
        immediate |= signbit << 15;
    }
    else if (opcode == 99) 
    {
        unsigned oneToFour = (input & 3840) >> 8;
        unsigned fiveToTen = (input & 2113929216) >> 25;
        unsigned eleven = (input & 128) >> 7;
        unsigned twelve = (input & 2147483648) >> 31;

        immediate |= oneToFour << 1;
        immediate |= fiveToTen << 5;
        immediate |= eleven << 10;
        immediate |= twelve << 11;
        immediate |= twelve << 12;
        immediate |= twelve << 13;
        immediate |= twelve << 14;
        immediate |= twelve << 15;

    }
    return (Signal)immediate;
}

// FIXME (4). ALU
void ALU(Signal input_0,
         Signal input_1,
         Signal ALU_ctrl_signal,
         Signal *ALU_result,
         Signal *zero)
{
    // For addition
    if (ALU_ctrl_signal == 2)
    {
        printf("ALU: %d + %d\n",input_0, input_1);
        *ALU_result = (input_0 + input_1);
        if (*ALU_result == 0) { *zero = 1; } else { *zero = 0; }
    }
    else if (ALU_ctrl_signal == 3)
    {
        printf("ALU: %d - %d\n",input_0, input_1);
        *ALU_result = (input_0 - input_1);
        if (*ALU_result == 0) { *zero = 1; } else { *zero = 0; }
    }
    else if (ALU_ctrl_signal == 4) //Custom slli logic
    {
        printf("ALU: %d << %d\n",input_0, input_1);
        *ALU_result = input_0 << input_1;
        if (*ALU_result == 0) { *zero = 1; } else { *zero = 0; }
    }
}

// (4). MUX
Signal MUX(Signal sel,
           Signal input_0,
           Signal input_1)
{
    //printf("MUX\n");
    if (sel == 0) { return input_0; } else { return input_1; }
}

// (5). Add
Signal Add(Signal input_0,
           Signal input_1)
{
    return (input_0 + input_1);
}

// (6). ShiftLeft1
Signal ShiftLeft1(Signal input)
{
    //printf("Left shift 1: %ld\n", (input << 1));
    return input << 1;
}
