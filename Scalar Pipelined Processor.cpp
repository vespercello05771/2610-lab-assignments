#include <bitset>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct IFstage {
    bitset<16> PC; //program counter
    bool nop;      //flag to represent whether the stage executes or not
};

struct IDstage {
    bitset<16> inst; //instruction
    bool nop;
};

struct EXstage {
    bitset<8> data1;          //data stored in first operand
    bitset<8> data2;          //data stored in second operand
    bitset<4> R1;             //first operand register
    bitset<4> R2;             //second operand register
    bitset<4> write_reg_addr; //destination register address
    bool rd_mem;              //to read from cache
    bool wrt_mem;             //to write to cache
    bool wrt_enable;          //to enable write in write back stage
    bool nop;
};

struct MEMStage {
    bitset<8> ALUresult;  //address to be written
    bitset<8> Store_data; //stores the result of alu
    bitset<4> R1;
    bitset<4> R2;
    bitset<4> write_reg_addr;
    bool rd_mem;
    bool wrt_mem;
    bool wrt_enable;
    bool nop;
};

struct WBstage {
    bitset<8> Wrt_data; //data to be written
    bitset<4> R1;
    bitset<4> R2;
    bitset<4> write_reg_addr;
    bool wrt_enable;
    bool nop;
};

struct stateStruct { //5 stage pipeline
    IFstage IF;
    IDstage ID;
    EXstage EX;
    MEMStage MEM;
    WBstage WB;
};

class RF {
private:
    vector<bitset<8>> Registers; //array of registers

public:
    bitset<8> Reg_data;
    RF() {
        Registers.resize(16); //since we have 16 registers
        ifstream rf;
        string line;
        rf.open("RF.txt");
        int result;
        if (rf.is_open()) {                //reading from RF.txt
            for (int i = 0; i < 16; i++) { //reading all the registers
                rf >> hex >> result;       //hexa decimals are converted to binary
                Registers[i] = bitset<8>(result);
            }
        } else
            cout << "Unable to open file"<<endl;
        rf.close();
    }

    bitset<8> readRF(bitset<4> Reg_addr) { //to read from a register
        Reg_data = Registers[Reg_addr.to_ulong()];
        return Reg_data;
    }

    void writeRF(bitset<4> Reg_addr, bitset<8> Wrt_reg_data) { //to write from a register
        Registers[Reg_addr.to_ulong()] = Wrt_reg_data;
    }
};

class INSMem {
private:
    vector<bitset<8>> IMem; //instruction cache

public:
    bitset<16> Instruction;
    INSMem() {
        IMem.resize(256);
        ifstream imem;
        int line;

        imem.open("ICache.txt");
        if (imem.is_open()) { //reading from ICache.txt
            for (int i = 0; i < 256; i++) {
                imem >> hex >> line;
                IMem[i] = bitset<8>(line);
            }
        } else
            cout << "Unable to open file"<<endl;
        imem.close();
    }

    bitset<16> readInstr(bitset<16> ReadAddress) { //function to read Instruction cache
        string insmem = "";
        insmem.append(IMem[ReadAddress.to_ulong()].to_string());
        insmem.append(IMem[ReadAddress.to_ulong() + 1].to_string());

        Instruction = bitset<16>(insmem);
        return Instruction;
    }

    void writeRF(bitset<16> i_addr, bitset<8> Wrt_data) { //to write from a register
        IMem[i_addr.to_ulong()] = bitset<8>(Wrt_data.to_string().substr(0, 8));
        IMem[i_addr.to_ulong() + 1] = bitset<8>(Wrt_data.to_string().substr(8, 8));
    }
};

class DataMem {
private:
    vector<bitset<8>> DMem; //data cahce

public:
    bitset<8> ReadData;
    DataMem() {
        DMem.resize(256); // resized
        ifstream dmem;
        int line;

        dmem.open("DCache.txt");
        if (dmem.is_open()) { //reading from DCache.txt
            for (int i = 0; i < 256; i++) {
                dmem >> hex >> line;
                DMem[i] = bitset<8>(line);
            }
        } else
            cout << "Unable to open file"<<endl;
        dmem.close();
    }

    bitset<8> readDataMem(bitset<8> Address) { //read data from cache
        string datamem = "";
        datamem.append(DMem[Address.to_ulong()].to_string());
        ReadData = bitset<8>(datamem);
        return ReadData;
    }

    void writeDataMem(bitset<8> Address, bitset<8> WriteData) { //writing data in cache
        DMem[Address.to_ulong()] = bitset<8>(WriteData.to_string().substr(0, 8));
    }
    void update_dmem() { //to print cache contains
        int temp;
        ofstream file;
        file.open("ODCache.txt");
        if (file.is_open()) {
            for (int i = 0; i < 256; i++) {
                temp = DMem[i].to_ulong();
                file << hex << (temp / 16) << (temp % 16) << endl;
            }
        } else
            cout << "Unable to open file"<<endl;
        file.close();
    }
};

unsigned long shiftbits(bitset<16> inst, int start) { //right shift the inst by the offset 'start'
    return ((inst) >> start).to_ulong();
}

int main() {
    RF myRF;           //register file
    INSMem myInsMem;   // ICache
    DataMem myDataMem; // DCache

    stateStruct state, newState; //tells the state of system by specifying all 5 stages of pipeline
    state.IF.PC = bitset<16>(0);
    state.IF.nop = 0;

    //flags which let us move to next tage of pipeline
    state.ID.nop = 1;
    state.EX.nop = 1;
    state.MEM.nop = 1;
    state.WB.nop = 1;

    //to specify stalls at each stage
    bool IFactive = true;
    bool MEMactive = true;
    bool EXactive = true;
    bool WBactive = true;

    newState = state;

    /-----------------------------initialisation of all fields in every stage-------------------------------------------/
    state.ID.inst = bitset<16>(0);
    state.EX.data1 = bitset<8>(0);
    state.EX.data2 = bitset<8>(0);
    state.EX.R1 = bitset<4>(0);
    state.EX.R2 = bitset<4>(0);
    state.EX.write_reg_addr = bitset<4>(0);
    state.EX.rd_mem = 0;
    state.EX.wrt_mem = 0;
    state.EX.wrt_enable = 0;
    state.MEM.ALUresult = bitset<8>(0);
    state.MEM.R1 = bitset<4>(0);
    state.MEM.R2 = bitset<4>(0);
    state.MEM.Store_data = bitset<8>(0);
    state.MEM.write_reg_addr = bitset<4>(0);
    state.MEM.rd_mem = 0;
    state.MEM.wrt_mem = 0;
    state.MEM.wrt_enable = 0;
    state.WB.R1 = bitset<4>(0);
    state.WB.R2 = bitset<4>(0);
    state.WB.write_reg_addr = bitset<4>(0);
    state.WB.Wrt_data = bitset<8>(0);
    state.WB.wrt_enable = 0;
    /----------------------------------------------------------------------------------/

    bitset<16> instruction = bitset<16>(0);
    bitset<4> opcode = bitset<4>(0);
    vector<bool> notAvailable(16, false); //array which tells us if a register is free(this is for RAW hazards)

    //computations we need to make
    int arithmetic_instructions = 0;
    int logical_instructions = 0;
    int data_instructions = 0;
    int control_instructions = 0;
    int halt_instructions = 0;
    int total_number_of_stalls = 0;
    int data_stalls = 0;
    int total_number_of_instructions_executed = 0;
    int control_stalls = 0;
    float cpi;

    int cycle = 0;

    //loop to execute pipeline
    while (1) {
        /---------------WB stage--------------------/
        if (!state.WB.nop && WBactive) {
            if (state.WB.wrt_enable) {
                myRF.writeRF(state.WB.write_reg_addr, state.WB.Wrt_data.to_ulong()); //writing into destination register
                notAvailable[state.WB.write_reg_addr.to_ulong()] = false;            //now that register is free
            }
        }
        WBactive = MEMactive;
        /---------------end WB stage--------------------/

        /---------------MEM stage--------------------/
        newState.WB.nop = state.MEM.nop; //to enable execution of next instruction

        if (!state.MEM.nop && MEMactive) {
            if (state.MEM.wrt_mem) {
                myDataMem.writeDataMem(state.MEM.ALUresult, state.MEM.Store_data); //store instruction
            }
            if (state.MEM.rd_mem) {
                newState.WB.Wrt_data = myDataMem.readDataMem(state.MEM.ALUresult); //load instruction
            } else {
                newState.WB.Wrt_data = state.MEM.ALUresult; //if Dcache is not involved
            }

            //storing operands for next stage execution
            newState.WB.R1 = state.MEM.R1;
            newState.WB.R2 = state.MEM.R2;
            newState.WB.write_reg_addr = state.MEM.write_reg_addr;
            newState.WB.wrt_enable = state.MEM.wrt_enable;
        }
        MEMactive = EXactive;
        /---------------end MEM stage--------------------/

        /---------------EX stage--------------------/
        newState.MEM.nop = state.EX.nop;

        if (!state.EX.nop && EXactive) {

            if (opcode.to_ulong() == 0) {
                newState.MEM.ALUresult = (state.EX.data1.to_ulong() + state.EX.data2.to_ulong()); //ADD
                arithmetic_instructions++;
            } else if (opcode.to_ulong() == 1) {
                newState.MEM.ALUresult = (state.EX.data1.to_ulong() - state.EX.data2.to_ulong()); //SUB
                arithmetic_instructions++;
            } else if (opcode.to_ulong() == 2) {
                newState.MEM.ALUresult = (state.EX.data1.to_ulong() * state.EX.data2.to_ulong()); //MUL
                arithmetic_instructions++;
            } else if (opcode.to_ulong() == 3) {
                newState.MEM.ALUresult = (state.EX.data1.to_ulong() + 1); //INC
                arithmetic_instructions++;
            } else if (opcode.to_ulong() == 4) {
                newState.MEM.ALUresult = (state.EX.data1 & state.EX.data2); //AND
                logical_instructions++;
            } else if (opcode.to_ulong() == 5) {
                newState.MEM.ALUresult = (state.EX.data1 | state.EX.data2); //OR
                logical_instructions++;
            } else if (opcode.to_ulong() == 6) { //NOT
                newState.MEM.ALUresult = (~state.EX.data1);
                logical_instructions++;
            } else if (opcode.to_ulong() == 7) {
                newState.MEM.ALUresult = (state.EX.data1 ^ state.EX.data2); //XOR
                logical_instructions++;
            } else if (opcode.to_ulong() == 8) {                                                //LOAD
                newState.MEM.ALUresult = state.EX.data1.to_ulong() + state.EX.data2.to_ulong(); //Effective address
                data_instructions++;
            } else if (opcode.to_ulong() == 9) {                                                //STORE
                newState.MEM.ALUresult = state.EX.data1.to_ulong() + state.EX.data2.to_ulong(); //Effective address
                newState.MEM.Store_data = myRF.readRF(state.EX.write_reg_addr);
                data_instructions++;
            }

            //storing operands for next stage execution
            newState.MEM.R1 = state.EX.R1;
            newState.MEM.R2 = state.EX.R2;
            newState.MEM.rd_mem = state.EX.rd_mem;
            newState.MEM.write_reg_addr = state.EX.write_reg_addr;
            newState.MEM.wrt_enable = state.EX.wrt_enable;
            newState.MEM.wrt_mem = state.EX.wrt_mem;
        }
        /---------------end EX stage--------------------/

        /---------------ID stage--------------------/
        newState.EX.nop = state.ID.nop;

        if (!state.ID.nop) {
            instruction = state.ID.inst;
            opcode = bitset<4>(shiftbits(instruction, 12));

            if (opcode.to_ulong() >= 0 && opcode.to_ulong() < 8) { //arithmetic or logical instructions
                newState.EX.write_reg_addr = bitset<4>(shiftbits(instruction, 8));
                if (opcode.to_ulong() != 3 && opcode.to_ulong() != 6) { //but not NOT and INC,then we have 3 operands in instruction
                    newState.EX.R1 = bitset<4>(shiftbits(instruction, 4));
                    newState.EX.R2 = bitset<4>(shiftbits(instruction, 0));

                    if (!notAvailable[newState.EX.R1.to_ulong()] && !notAvailable[newState.EX.R2.to_ulong()]) { //checking for availability of registers for RAW hazards

                        notAvailable[newState.EX.write_reg_addr.to_ulong()] = true;
                        bitset<8> data1 = myRF.readRF(newState.EX.R1); //reading data from registers of register file
                        bitset<8> data2 = myRF.readRF(newState.EX.R2);

                        newState.EX.data1 = bitset<8>(data1[7] ? (-256 + data1.to_ulong()) : data1.to_ulong()); //if data is -ve
                        newState.EX.data2 = bitset<8>(data2[7] ? (-256 + data2.to_ulong()) : data2.to_ulong()); //if data is +ve
                        newState.EX.rd_mem = false;                                                             //not LOAD instruction
                        newState.EX.wrt_mem = false;                                                            //not STORE instruction
                        newState.EX.wrt_enable = true;
                    }
                } else {
                    if (opcode.to_ulong() != 3) {                              //NOT instruction
                        newState.EX.R1 = bitset<4>(shiftbits(instruction, 4)); //last 4 bits discarded, only 2 operands
                    } else {
                        newState.EX.R1 = bitset<4>(shiftbits(instruction, 8)); //last 8 bits discarded, only 1 operand
                    }

                    newState.EX.write_reg_addr = bitset<4>(shiftbits(instruction, 8));

                    if (!notAvailable[newState.EX.R1.to_ulong()]) { // if R1 is free, then it can be used in operation
                        notAvailable[newState.EX.write_reg_addr.to_ulong()] = true;
                        bitset<8> data1 = myRF.readRF(newState.EX.R1);
                        newState.EX.data1 = bitset<8>(data1[7] ? (-256 + data1.to_ulong()) : data1.to_ulong());
                        newState.EX.wrt_enable = true;
                        newState.EX.rd_mem = false;  //not LOAD instruction
                        newState.EX.wrt_mem = false; //not STORE instruction
                        IFactive = true;
                        EXactive = true;
                    } else {
                        //Stalls
                        IFactive = false;
                        EXactive = false;
                    }
                }
            } else if (opcode.to_ulong() == 8) { //LOAD operation

                newState.EX.R1 = bitset<4>(shiftbits(instruction, 4));
                newState.EX.write_reg_addr = bitset<4>(shiftbits(instruction, 8)); //data from [R1 + R2] will be stored here

                if (!notAvailable[newState.EX.R1.to_ulong()]) { //if R1 is free only then we can proceed
                    notAvailable[newState.EX.R1.to_ulong()] = true;
                    newState.EX.R2 = bitset<4>(shiftbits(instruction, 0)); //offset

                    newState.EX.data1 = myRF.readRF(newState.EX.R1); //taking data from registers
                    newState.EX.data2 = myRF.readRF(newState.EX.R2);
                    newState.EX.wrt_enable = false;
                    newState.EX.rd_mem = true; //this is LOAD so we make this read into memory flag 1
                    newState.EX.wrt_mem = false;
                    IFactive = true;
                    EXactive = true;
                } else {
                    total_number_of_stalls++;
                    data_stalls++;
                    IFactive = false;
                    EXactive = false;
                }
            } else if (opcode.to_ulong() == 9) { //STORE operation
                newState.EX.R1 = bitset<4>(shiftbits(instruction, 4));
                newState.EX.write_reg_addr = bitset<4>(shiftbits(instruction, 8)); //data from here will be stored in [R1 + R2]

                if (!notAvailable[newState.EX.R1.to_ulong()] && !notAvailable[newState.EX.write_reg_addr.to_ulong()]) {

                    newState.EX.R2 = bitset<4>(shiftbits(instruction, 0)); //offset

                    newState.EX.data1 = myRF.readRF(newState.EX.R1); //taking data from registers
                    newState.EX.data2 = myRF.readRF(newState.EX.R2);
                    newState.EX.wrt_enable = false;
                    newState.EX.rd_mem = false;
                    newState.EX.wrt_mem = true; //this is LOAD instruction is we enable writing into memory flag
                    IFactive = true;
                    EXactive = true;

                } else {
                    total_number_of_stalls++;
                    data_stalls++;
                    IFactive = false;
                    EXactive = false;
                }

            } else if (opcode.to_ulong() == 10) { //UNCONDITIONAL JUMP
                control_instructions++;
                bitset<8> L1 = bitset<8>(shiftbits(instruction, 4)); //offset from PC
                if (L1[7]) {
                    state.IF.PC = bitset<16>(state.IF.PC.to_ulong() + ((L1.to_ulong() - 256) << 1)); // -ve L1
                } else {
                    state.IF.PC = bitset<16>(state.IF.PC.to_ulong() + (L1.to_ulong() << 1)); //+ve L1
                }
                newState.EX.nop = 1;
            } else if (opcode.to_ulong() == 11) { //BEQZ - Conditional jump

                /*here we implemented BEQZ in ID stage itself because this leads to lesser no. of memory stalls and easier execution,this is
            the reason why we are getting different answer for stalls and CPI than given in the sample "Output.txt"*/

                bitset<8> L1 = bitset<8>(shiftbits(instruction, 0)); //offset
                state.EX.R1 = bitset<4>(shiftbits(instruction, 8));  //register for condition checking

                if (!notAvailable[newState.EX.R1.to_ulong()]) {
                    control_instructions++;
                    if (myRF.readRF(newState.EX.R1).to_ulong() == 0) { //if R1==0, then Jump
                        if (L1[7]) {
                            state.IF.PC = bitset<16>(state.IF.PC.to_ulong() + ((L1.to_ulong() - 256) << 1)); // -ve L1
                        } else {
                            state.IF.PC = bitset<16>(state.IF.PC.to_ulong() + (L1.to_ulong() << 1)); // +ve L1
                        }
                        newState.EX.nop = 1; //since jump needs to be done, we need to disable the flag leading to next stage execution
                    }
                    IFactive = true;
                    EXactive = true;
                } else {
                    total_number_of_stalls++;
                    control_stalls++;
                    IFactive = false;
                    EXactive = false;
                }
                newState.EX.nop = 1;
            }
        }

        newState.ID.nop = state.IF.nop;
        /---------------end ID stage--------------------/

        /---------------IF stage-------------------------------------------/

        if (!state.IF.nop && IFactive) { //instruction fetch happens only when flag state.IF.nop is set to 0 and IFactive is active
            total_number_of_instructions_executed++;

            newState.ID.inst = myInsMem.readInstr(state.IF.PC); //reading instruction from cache
            newState.IF.PC = state.IF.PC.to_ulong() + 2;        //PC is incremented

            if (newState.ID.inst.to_string().substr(0, 4) == "1111") { //halt
                newState.IF.nop = true;
                newState.ID.nop = true;
                halt_instructions++;
            }
        }
        /---------------end IF stage--------------------/
        //if flages of all the stages are set to zero then we break out
        if (state.IF.nop && state.ID.nop && state.EX.nop && state.MEM.nop && state.WB.nop) {
            break;
        }
        state = newState;
        cycle++;
        cout<<"HERE IS THE CYCLE "<<cycle<<endl;
    }
    myDataMem.update_dmem();
    cout << cycle << endl;
    cpi = ((float)cycle / total_number_of_instructions_executed);

    // result file for counters
    ofstream Output;
    Output.open("Output.txt");
    Output << "Total number of instructions executed: " << total_number_of_instructions_executed << endl;
    Output << "Number of instructions in each class" << endl;
    Output << "Arithmetic instructions              : " << arithmetic_instructions << endl;
    Output << "Logical instructions                 : " << logical_instructions << endl;
    Output << "Data instructions                    : " << data_instructions << endl;
    Output << "Control instructions                 : " << control_instructions << endl;
    Output << "Halt instructions                    : " << halt_instructions << endl;
    Output << "Cycles Per Instruction               : " << cpi << endl;
    Output << "Total number of stalls               : " << total_number_of_stalls << endl;
    Output << "Data stalls (RAW)                    : " << data_stalls << endl;
    Output << "Control stalls                       : " << control_stalls << endl;
    Output.close();
    return 0;
}