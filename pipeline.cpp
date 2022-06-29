#include <math.h>
#include <stdlib.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
using namespace std;

string instructions[1000];  // save the instructios
string
    fetchedInsts[2500];  // save the instructions in the order they were fetched
int numInstFetches = 0;  // used as index for the above and 2 below arrays
int instFetchCycle[2500];  // save the cycle the instruction was fetched on
int instWBCycle[2500];     // save the cycle the instruction was written back on
int instOPCode[2500];      // save the instruction's op code
int instRegSrc1[2500];     // rs
int instRegSrc2[2500];     // rt or indicates immediate
int instRegDst[2500];      // rd or rt if immediate
bool instIsMemory[2500];
bool instIsStore[2500];
bool instIsLoad[2500];
bool instIsBranch[2500];
string instMemAddress[2500];
bool WBState[2500];  // true if instruction has been written back already,
                     // prevents double write backs
int pipelineStalls[] = {0, 0, 0, 0, 0};
int numLWHazards = 0;
// index 0: stalls in fetch ... index 4: stalls in write back
int numFlushes = 0;  // keeps track of flushes to keep everything on track
int numDelays = 0;   // number of additional cycles spent in execution
int mem[5];          // simulated the memory
int reg[10];         // simulated the register
int clockcycle = 0;  // simulated the clock cycle
ofstream fout("Result.txt");

/// deal with hazard
bool lwhz = false;
bool bneq = false;
bool flu = false;

/// for instruction fetch variable
int pc = 0;
bool ifid_input = false;
string finstruction = "00000000000000000000000000000000";

/// for insruction decode and register fectch variable
int idpc = 0;
string op = "000000";
string fun = "000000";
string idexsignal = "000000000";
bool idex_input = 0;
int readdata1 = 0;
int readdata2 = 0;
int signextend = 0;
long int rs = 0;
int rt = 0;
int rd = 0;

/// for execution variable
bool exe_input = false;
int aluout = 0;
int writedata = 0;
int exert = 0;
int exepc = 0;
int temp = 0;
int temp2 = 0;
string exesignal = "000000000";
string aluop = "00";

/// for read or write memory variable
bool meminput = false;
bool membranch = false;
int memreaddata = 0;
int memalu = 0;
int memrt = 0;
string memsignal = "00";
/// for wb
int wbrd = 0;

int b_d(string str, bool sign)  // binary to decimal
{
  bool f = false;
  int ans = 0;
  for (int i = 0; i < str.length(); i++) {
    ans = ans * 2 + str[i] - '0';
    if (i == 0 && str[i] == '1') f = true;
  }
  if (f && sign)  // if it is a signinteger
  {
    ans = -(ans);
  }
  return ans;
}
void print();

void flush() {
  // run the bne hazard happen clock cycle with this function
  ifid_input = true;
  pc = exepc + 4;
  instMemAddress[numInstFetches - 3] = exepc;
  numFlushes++;
  if (instructions[(pc / 4)] == "")  // pc/4= the number of instruction
  {
    finstruction = "00000000000000000000000000000000";  // no instruction
    ifid_input = false;
  } else {
    finstruction = instructions[(pc / 4)];  // fetch the instruction
  }
  instWBCycle[numInstFetches - 2] = -999 - numDelays;
  instWBCycle[numInstFetches - 1] =
      -999 -
      numDelays;  // indicates the last 2 instructions do not get executed

  instFetchCycle[numInstFetches] = clockcycle + numDelays + numFlushes -
                                   1;  // sets up the newly fetched instruction
  fetchedInsts[numInstFetches] = finstruction;
  numInstFetches++;
  // id_exe
  idex_input = true;
  readdata1 = 0;
  readdata2 = 0;
  signextend = 0;
  rs = 0;
  rt = 0;
  rd = 0;
  idpc = 0;
  idexsignal = "000000000";
  print();
}
void detect() {
  if (exert == rs && exert != 0 &&
      exesignal[3] == '1')  // forwarding for data hazard A 10
  {
    readdata1 = aluout;
    // if (instIsLoad[numInstFetches - 2] == false) {
    numDelays += 2;
    instWBCycle[numInstFetches - 4] -= 2;
    instWBCycle[numInstFetches - 3] -= 2;
    // these instructions are ahead in the pipeline - unaffected (wb runs
    // first so is unaffected)
    instFetchCycle[numInstFetches] -= 2;
    // the instruction fetched this cycle gets stuck in the pipeline
    pipelineStalls[1] += 2;
    //}
  }
  if (exert == rt && exert != 0 && exesignal[3] == '1')  // forward B 10
  {
    readdata2 = aluout;
    if (instIsLoad[numInstFetches - 2] == false) {
      numDelays += 2;  // lw writes to rt so no RAW hazard
      instWBCycle[numInstFetches - 4] -= 2;
      instWBCycle[numInstFetches - 3] -= 2;
      // these instructions are ahead in the pipeline - unaffected (wb runs
      // first so is unaffected)
      instFetchCycle[numInstFetches] -= 2;
      // the instruction fetched this cycle gets stuck in the pipeline
      pipelineStalls[1] += 2;
    }
  }

  if (memrt == rs && memrt != 0 && memsignal[0] == '1') {
    readdata1 = (memsignal[1] == '1') ? memreaddata : memalu;  // ForwardA01
    if (instIsLoad[numInstFetches - 2] == false) {
      numDelays += 1;
      instWBCycle[numInstFetches - 4] -= 1;
      instWBCycle[numInstFetches - 3] -= 1;
      // these instructions are ahead in the pipeline - unaffected (wb runs
      // first so is unaffected)
      instFetchCycle[numInstFetches] -= 1;
      // the instruction fetched this cycle gets stuck in the pipeline
      pipelineStalls[1] += 1;
    }
  }
  if (memrt == rt && memrt != 0 && memsignal[0] == '1') {
    readdata2 = (memsignal[1] == '1') ? memreaddata : memalu;  // ForwardB01
    if (instIsStore[numInstFetches - 3] == false &&
        instIsLoad[numInstFetches - 3] == false) {
      numDelays += 1;  // there is no RAW depedency on stores
      instWBCycle[numInstFetches - 4] -= 1;
      instWBCycle[numInstFetches - 3] -= 1;
      // these instructions are ahead in the pipeline - unaffected (wb runs
      // first so is unaffected)
      instFetchCycle[numInstFetches] -= 1;
      // the instruction fetched this cycle gets stuck in the pipeline
      pipelineStalls[1] += 1;
    }
  }
  if (idexsignal[5] == '1' &&
      (rt == b_d(finstruction.substr(6, 5), false) ||
       rt == b_d(finstruction.substr(11, 5), false)))  // lw hazard
  {
    // set NOP
    lwhz = true;
    pc -= 4;
    ifid_input = true;
    numLWHazards++;
    instWBCycle[numInstFetches - 2] -= 1;
    if (numInstFetches >= 4) {
      instWBCycle[numInstFetches - 3] -= 1;
      instWBCycle[numInstFetches - 4] -= 1;
    } else if (numInstFetches >= 3)
      instWBCycle[numInstFetches - 3] -= 1;
    instFetchCycle[numInstFetches - 1] = -1;
    numInstFetches--;
    //  fout << "lw hazard\n";
  }
}
void writeback(string wbsignal, int rt, int alu, int readata) {
  if (WBState[numInstFetches + numFlushes - 4] == false) {
    if (instWBCycle[numInstFetches - numFlushes - 4] >= -5 - numDelays) {
      instWBCycle[numInstFetches - numFlushes - 4] +=
          clockcycle + numDelays + numLWHazards;
      WBState[numInstFetches + numFlushes - 4] = true;
    }
  }             // if WB set to -999 don't change it (for above if statement)
  if (rt == 0)  //$0 cant overwrite
    return;
  if (wbsignal[0] == '1') {
    if (wbsignal[1] == '0')  // R-type
    {
      reg[rt] = alu;
    } else if (wbsignal[1] == '1')  // lw
    {
      reg[rt] = readata;
    }
  } else if (wbsignal[0] == '0')  // sw and beq
  {
    // NOP
    return;
  }
}
void instructiondecode() {
  idex_input = ifid_input;
  ifid_input = false;
  idpc = pc;
  op = finstruction.substr(0, 6);  // first 6 bits
  instOPCode[numInstFetches + numLWHazards - 1] = b_d(op, false);
  if (numLWHazards > 0) numLWHazards--;
  if (op == "000000")  // r type
  {
    rs = b_d(finstruction.substr(6, 5), false);
    rt = b_d(finstruction.substr(11, 5), false);
    rd = b_d(finstruction.substr(16, 5), false);
    fun = finstruction.substr(26);
    instRegDst[numInstFetches - 1] = rd;
    instRegSrc1[numInstFetches - 1] = rs;
    instRegSrc2[numInstFetches - 1] = rt;
    readdata1 = reg[rs];
    readdata2 = reg[rt];
    // r type no singextend
    signextend = b_d(finstruction.substr(16, 16), true);
    if (rs == 0 && rt == 0 && rd == 0 && fun == "000000" ||
        lwhz == true)  // no instruction
    {
      idexsignal = "000000000";
      lwhz = false;

    } else {
      idexsignal = "110000010";
    }

  } else  // i type
  {
    rs = b_d(finstruction.substr(6, 5), false);
    rt = b_d(finstruction.substr(11, 5), false);
    readdata1 = reg[rs];
    readdata2 = reg[rt];
    instRegDst[numInstFetches - 1] = rt;
    instRegSrc1[numInstFetches - 1] = rs;
    instRegSrc2[numInstFetches - 1] =
        -1;  // immediate function so no rt/regsrc2
    signextend = b_d(finstruction.substr(16, 16), true);
    // i type no
    rd = 0;
    if (op == "100011")  // lw
    {
      idexsignal = "000101011";
      instIsMemory[numInstFetches - 1] = true;
      instIsLoad[numInstFetches - 1] = true;
      instRegSrc1[numInstFetches - 1] = -1;
    } else if (op == "101011")  // sw
    {
      idexsignal = "100100101";
      instIsMemory[numInstFetches - 1] = true;
      instIsStore[numInstFetches - 1] = true;
      instRegDst[numInstFetches - 1] = -1;
      instRegSrc1[numInstFetches - 1] = rt;
    } else if (op == "001000")  // addi
    {
      idexsignal = "000100010";
    } else if (op == "001100")  // andi
    {
      idexsignal = "011100010";
    } else if (op == "000101")  // bne
    {
      idexsignal = "101010001";
      instIsBranch[numInstFetches - 1] = true;
    }
  }
}
void execution() {
  exe_input = idex_input;
  idex_input = false;

  aluop = idexsignal.substr(1, 2);
  exesignal = idexsignal.substr(4, 5);
  writedata = readdata2;                             // for sw, not always use
  if (idexsignal[0] == '1' && idexsignal[1] == '1')  // r type
  {
    exert = rd;
  } else {
    exert = rt;
  }
  temp = readdata1;
  if (idexsignal[3] == '0')  // alusrc== 0
    temp2 = readdata2;
  else  // alusrc==0
    temp2 = signextend;
  if (aluop == "10")  // r type
  {
    if (fun == "100000")  // add
      aluout = temp + temp2;
    else if (fun == "100100")  // and
      aluout = temp & temp2;
    else if (fun == "100101")  // or
      aluout = temp | temp2;
    else if (fun == "100010")  // sub, I made this take 3 cycles as an example
    {
      aluout = temp - temp2;
      numDelays += 2;
      instWBCycle[numInstFetches - 3] -=
          2;  // this instruction is ahead in the pipeline - unaffected (wb runs
              // first so is unaffected)
      // the instruction fetched this cycle gets stuck in pipeline
      instFetchCycle[numInstFetches] -= 2;
      pipelineStalls[2] += 2;
    } else if (fun == "101010")  // slt
    {
      if (temp < temp2)
        aluout = 1;
      else
        aluout = 0;
    } else if (fun == "000000")
      aluout = 0;
  } else if (aluop == "00")  // lw&sw
    aluout = temp + temp2;
  else if (aluop == "01")  // bNeq
  {
    aluout = temp - temp2;
    bneq = true;
    if (aluout != 0) flu = true;
  }

  else if (idexsignal == "000100010")  // addi
  {
    aluout = temp + temp2;

  } else if (idexsignal == "011100010")  // andi
  {
    aluout = temp & temp2;
  }
  exepc = idpc + (signextend * 4);
}
void memoryaccess() {
  meminput = exe_input;
  exe_input = false;
  membranch = false;
  memrt = exert;
  memalu = aluout;
  memsignal = exesignal.substr(3, 2);
  if (memsignal == "01" && memalu != 0 && bneq == true)  // bneq
  {
    bneq = false;
    membranch = true;
  }
  if (memsignal == "11")  // load
  {
    memreaddata = mem[memalu / 4];
  } else {
    memreaddata = 0;
  }
  if (memsignal == "01" && exesignal[2] == '1')  // sw
  {
    mem[memalu / 4] = writedata;
  }
}
void instructionfetch() {
  instFetchCycle[numInstFetches] += clockcycle + numFlushes + numDelays;
  pc = pc + 4;
  ifid_input = true;
  if (instructions[(pc / 4)] == "")  // pc/4= the number of instruction
  {
    finstruction = "00000000000000000000000000000000";  // no instruction
    ifid_input = false;
  } else {
    finstruction = instructions[(pc / 4)];  // fetch the instruction
  }
  fetchedInsts[numInstFetches] = finstruction;
  numInstFetches++;
}
bool gocycling() {
  wbrd = memrt;
  writeback(memsignal, memrt, memalu, memreaddata);
  memoryaccess();
  execution();
  instructiondecode();
  instructionfetch();
  clockcycle++;
  wbrd = memrt;
  // beq hazard
  if (flu == true) {
    flu = false;
    flush();
  } else {
    print();
  }
  detect();

  if ((memsignal == "00" && memalu == 0 && memreaddata == 0) &&
      exesignal == "00000" && idexsignal == "000000000" && clockcycle != 1) {
    return false;
  } else  // continue
    return true;
}
int main() {
  // initialize the memory and registers
  mem[0] = 5;
  mem[1] = 9;
  mem[2] = 4;
  mem[3] = 8;
  mem[4] = 7;

  reg[0] = 0;
  reg[1] = 9;
  reg[2] = 8;
  reg[3] = 7;
  reg[4] = 1;
  reg[5] = 2;
  reg[6] = 3;
  reg[7] = 4;
  reg[8] = 5;
  reg[9] = 6;

  for (int j = 0; j < 2500; j++) {
    instWBCycle[j] = 0;
    instFetchCycle[j] = 0;
    //  these values are used in addition so they need to be initialized
    instIsMemory[j] = false;
    instIsStore[j] = false;
    instIsLoad[j] = false;
    instIsBranch[j] = false;
    WBState[j] = false;
    instMemAddress[j] = "0";
    // set defaults here so they only need to be updated if true/used
  }
  // file open
  ifstream fin("Input.txt");
  int i = 1;  // start with 1 for convient
  // read the input instructions
  while (fin >> instructions[i]) i++;

  // run by function gocycking
  while (gocycling() == true)
    ;
  for (int j = 0; j < numFlushes; j++) {
    gocycling();  // needs to run a little extra if there were flushes
  }

  // print instruction execution times
  // fetchedInsts[j] << "," <<
  for (int j = 0; j < clockcycle; j++) {
    if (instWBCycle[j] >= instFetchCycle[j]) {
      fout << instFetchCycle[j] + 1 << "," << instWBCycle[j] + 1 << ","
           << instOPCode[j] << "," << instRegDst[j] << "," << instRegSrc1[j]
           << "," << instRegSrc2[j] << "," << instIsStore[j] << ","
           << instIsLoad[j] << "," << instIsBranch[j] << ","
           << (instWBCycle[j] - instFetchCycle[j]) + 1 << endl;
    }
  }

  fin.close();
  fout.close();
  return 0;
}
void print() {
  /* remove comment to enable

  fout << "CC " << clockcycle << ":" << endl;
  fout << endl;
  fout << "Registers:" << endl;
  for (int k = 0; k < 10; k++) fout << "$" << k << ": " << reg[k] << endl;
  fout << endl;
  fout << "Data memory:" << endl;
  fout << "0x00: " << mem[0] << endl;
  fout << "0x04: " << mem[1] << endl;
  fout << "0x08: " << mem[2] << endl;
  fout << "0x0C: " << mem[3] << endl;
  fout << "0x10: " << mem[4] << endl;
  fout << endl;
  fout << "IF/ID :" << endl;
  fout << "PC              " << pc << endl;
  fout << "Instruction     " << finstruction << endl;
  fout << endl;
  fout << "ID/EX :" << endl;
  fout << "ReadData1       " << readdata1 << endl;
  fout << "ReadData2       " << readdata2 << endl;
  fout << "sign_ext        " << signextend << endl;
  fout << "Rs              " << rs << endl;
  fout << "Rt              " << rt << endl;
  fout << "Rd              " << rd << endl;
  fout << "Control signals " << idexsignal << endl;
  fout << endl;
  fout << "EX/MEM :" << endl;
  fout << "ALUout          " << aluout << endl;
  fout << "WriteData       " << writedata << endl;
  fout << "Rt/Rd           ";
  fout << exert << endl;
  fout << "Control signals " << exesignal << endl;
  fout << endl;
  fout << "MEM/WB :" << endl;
  fout << "ReadData        " << memreaddata << endl;
  fout << "ALUout          " << memalu << endl;
  fout << "Rt/Rd           " << wbrd << endl;
  fout << "Control signals " << memsignal << endl;
  fout << "================================================================="
       << endl;
       */
}
