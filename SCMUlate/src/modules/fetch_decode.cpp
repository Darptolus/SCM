#include "fetch_decode.hpp"
#include <string> 
#include <vector> 

scm::fetch_decode_module::fetch_decode_module(inst_mem_module * const inst_mem, reg_file_module * const reg_file_m, control_store_module * const control_store_m, mem_interface_module * const mem_int, bool * const aliveSig):
  inst_mem_m(inst_mem),
  reg_file_m(reg_file_m),
  ctrl_st_m(control_store_m),
  mem_interface_m(mem_int),
  aliveSignal(aliveSig),
  PC(0) { }

int
scm::fetch_decode_module::behavior() {
  SCMULATE_INFOMSG(1, "I am an SU");
  // Initialization barrier
  #pragma omp barrier
    while (*(this->aliveSignal)) {
      std::string current_instruction = this->inst_mem_m->fetch(this->PC);
      SCMULATE_INFOMSG(3, "I received instruction: %s", current_instruction.c_str());
      scm::decoded_instruction_t* cur_inst = scm::instructions::findInstType(current_instruction);
      SCMULATE_ERROR_IF(0, !cur_inst, "Returned instruction is NULL. This should not happen");
      // Depending on the instruction do something 
      switch(cur_inst->getType()) {
        case COMMIT:
          SCMULATE_INFOMSG(4, "I've identified a COMMIT");
          SCMULATE_INFOMSG(1, "Turning off machine alive = false");
          #pragma omp atomic write
          *(this->aliveSignal) = false;
          delete cur_inst;
          break;
        case CONTROL_INST:
          SCMULATE_INFOMSG(4, "I've identified a CONTROL_INST");
          executeControlInstruction(cur_inst);
          delete cur_inst;
          break;
        case BASIC_ARITH_INST:
          SCMULATE_INFOMSG(4, "I've identified a BASIC_ARITH_INST");
          executeArithmeticInstructions(cur_inst);
          delete cur_inst;
          break;
        case EXECUTE_INST:
          SCMULATE_INFOMSG(4, "I've identified a EXECUTE_INST");
          assignExecuteInstruction(cur_inst);
          delete cur_inst;
          break;
        case MEMORY_INST:
          SCMULATE_INFOMSG(4, "I've identified a MEMORY_INST");
          mem_interface_m->assignInstSlot(cur_inst);
          // Waiting for the instruction to finish execution
          while (!mem_interface_m->isInstSlotEmpty());
          break;
        default:
          SCMULATE_ERROR(0, "Instruction not recognized [%s]", current_instruction.c_str());
          #pragma omp atomic write
          *(this->aliveSignal) = false;
          delete cur_inst;
          break;
      }
      this->PC++;

    } 
    SCMULATE_INFOMSG(1, "Shutting down fetch decode unit");
    return 0;
}

void
scm::fetch_decode_module::executeControlInstruction(scm::decoded_instruction_t * inst) {

  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE JMPLBL INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "JMPLBL") {
    int newPC = this->inst_mem_m->getMemoryLabel(inst->getOp1()) - 1;
    SCMULATE_ERROR_IF(0, newPC == -2,   "Incorrect label translation");
    PC = newPC;
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE JMPPC INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "JMPPC") {
    int offset = std::stoi(inst->getOp1());
    int target = offset + PC - 1;
    SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0),  "Incorrect destination offset");
    PC = target;
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BREQ INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BREQ" ) {
    decoded_reg_t reg1 = instructions::decodeRegister(inst->getOp1());
    decoded_reg_t reg2 = instructions::decodeRegister(inst->getOp2());
    unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    unsigned char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool bitComparison = true;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    for (uint32_t i = 0; i < this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size); ++i) {
      if (reg1_ptr[i] ^ reg2_ptr[i]) {
        bitComparison = false;
        break;
      }
    }
    if (bitComparison) {
      int offset = std::stoi(inst->getOp3());
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0),  "Incorrect destination offset");
      PC = target;
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BGT INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BGT" ) {
    decoded_reg_t reg1 = instructions::decodeRegister(inst->getOp1());
    decoded_reg_t reg2 = instructions::decodeRegister(inst->getOp2());
    unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    unsigned char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool reg1_gt_reg2 = false;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    for (uint32_t i = 0; i < this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size); ++i) {
      // Find the first byte from MSB to LSB that is different in reg1 and reg2. If reg1 > reg2 in that byte, then reg1 > reg2 in general
      if (reg1_ptr[i] ^ reg2_ptr[i] && reg1_ptr[i] > reg2_ptr[i]) {
        reg1_gt_reg2 = true;
        break;
      }
    }
    if (reg1_gt_reg2) {
      int offset = std::stoi(inst->getOp3());
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0),  "Incorrect destination offset");
      PC = target;
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BGET INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BGET" ) {
    decoded_reg_t reg1 = instructions::decodeRegister(inst->getOp1());
    decoded_reg_t reg2 = instructions::decodeRegister(inst->getOp2());
    unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    unsigned char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool reg1_get_reg2 = false;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    uint32_t size_reg_bytes = this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size);
    for (uint32_t i = 0; i < size_reg_bytes; ++i) {
      // Find the first byte from MSB to LSB that is different in reg1 and reg2. If reg1 > reg2 in that byte, then reg1 > reg2 in general
      if (reg1_ptr[i] ^ reg2_ptr[i] && reg1_ptr[i] > reg2_ptr[i]) {
        reg1_get_reg2 = true;
        break;
      }
      // If we have not found any byte that is different in both registers from MSB to LSB, and the LSB byte is the same, the the registers are the same
      if (i == size_reg_bytes-1  && reg1_ptr[i] == reg2_ptr[i])
        reg1_get_reg2 = true;
    }
    if (reg1_get_reg2) {
      int offset = std::stoi(inst->getOp3());
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0),  "Incorrect destination offset");
      PC = target;
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BLT INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BLT" ) {
    decoded_reg_t reg1 = instructions::decodeRegister(inst->getOp1());
    decoded_reg_t reg2 = instructions::decodeRegister(inst->getOp2());
    unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    unsigned char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool reg1_lt_reg2 = false;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    for (uint32_t i = 0; i < this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size); ++i) {
      // Find the first byte from MSB to LSB that is different in reg1 and reg2. If reg1 < reg2 in that byte, then reg1 < reg2 in general
      if (reg1_ptr[i] ^ reg2_ptr[i] && reg1_ptr[i] < reg2_ptr[i]) {
        reg1_lt_reg2 = true;
        break;
      }
    }
    if (reg1_lt_reg2) {
      int offset = std::stoi(inst->getOp3());
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0),  "Incorrect destination offset");
      PC = target;
    }
    return;
  }
  /////////////////////////////////////////////////////
  ///// CONTROL LOGIC FOR THE BLET INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "BLET" ) {
    decoded_reg_t reg1 = instructions::decodeRegister(inst->getOp1());
    decoded_reg_t reg2 = instructions::decodeRegister(inst->getOp2());
    unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number);
    unsigned char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number);
    SCMULATE_INFOMSG(4, "Comparing register %s %d to %s %d", reg1.reg_size.c_str(), reg1.reg_number, reg2.reg_size.c_str(), reg2.reg_number);
    bool reg1_let_reg2 = false;
    SCMULATE_ERROR_IF(0, reg1.reg_size != reg2.reg_size, "Attempting to compare registers of different size");
    uint32_t size_reg_bytes = this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size);
    for (uint32_t i = 0; i < size_reg_bytes; ++i) {
      // Find the first byte from MSB to LSB that is different in reg1 and reg2. If reg1 < reg2 in that byte, then reg1 < reg2 in general
      if (reg1_ptr[i] ^ reg2_ptr[i] && reg1_ptr[i] < reg2_ptr[i]) {
        reg1_let_reg2 = true;
        break;
      }
      // If we have not found any byte that is different in both registers from MSB to LSB, and the LSB byte is the same, the the registers are the same
      if (i == size_reg_bytes-1 && reg1_ptr[i] == reg2_ptr[i])
        reg1_let_reg2 = true;
    }
    if (reg1_let_reg2) {
      int offset = std::stoi(inst->getOp3());
      int target = offset + PC - 1;
      SCMULATE_ERROR_IF(0, ((uint32_t)target > this->inst_mem_m->getMemSize() || target < 0),  "Incorrect destination offset");
      PC = target;
    }
    return;
  }

}
void
scm::fetch_decode_module::executeArithmeticInstructions(scm::decoded_instruction_t * inst) {
  /////////////////////////////////////////////////////
  ///// ARITHMETIC LOGIC FOR THE ADD INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "ADD") {
    decoded_reg_t reg1 = instructions::decodeRegister(inst->getOp1());
    decoded_reg_t reg2 = instructions::decodeRegister(inst->getOp2());
    // Second operand may be register or immediate. We assumme immediate are no longer than a long long
    if (!instructions::isRegister(inst->getOp3())) {
      // IMMEDIATE ADDITION CASE
      // TODO: Think about the signed option of these operands
      unsigned long long immediate_val = std::stoull(inst->getOp3());

      unsigned char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number); 

      // Where to store the result
      unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number); 
      int32_t size_reg_bytes = this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size);

      // Addition
      uint32_t temp = 0;
      for (int32_t i = size_reg_bytes-1; i >= 0; --i) {
        temp += (immediate_val & 255) + (reg2_ptr[i]); 
        reg1_ptr[i] = temp & 255;
        immediate_val >>= 8;
        // Carry on
        temp = temp > 255 ? 1: 0;
      }
    } else {
      // REGISTER REGISTER ADD CASE
      decoded_reg_t reg3 = instructions::decodeRegister(inst->getOp3());
      unsigned char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number); 
      unsigned char * reg3_ptr = this->reg_file_m->getRegisterByName(reg3.reg_size, reg3.reg_number); 

      // Where to store the result
      unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number); 
      int32_t size_reg_bytes = this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size);

      // Addition
      int temp = 0;
      for (int32_t i = size_reg_bytes-1; i >= 0; --i) {
        temp +=  (reg3_ptr[i]) + (reg2_ptr[i]); 
        reg1_ptr[i] = temp & 255;
        // Carry on
        temp = temp > 255? 1: 0;
      }
    }
    return;
  }

  /////////////////////////////////////////////////////
  ///// ARITHMETIC LOGIC FOR THE SUB INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "SUB") {
    decoded_reg_t reg1 = instructions::decodeRegister(inst->getOp1());
    decoded_reg_t reg2 = instructions::decodeRegister(inst->getOp2());

    // Second operand may be register or immediate. We assumme immediate are no longer than a long long
    if (!instructions::isRegister(inst->getOp3())) {
      // IMMEDIATE ADDITION CASE
      // TODO: Think about the signed option of these operands
      unsigned long long immediate_val = std::stoull(inst->getOp3());

      unsigned char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number); 

      // Where to store the result
      unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number); 
      int32_t size_reg_bytes = this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size);

      // Subtraction
      uint32_t temp = 0;
      for (int32_t i = size_reg_bytes-1; i >= 0; --i) {
        uint32_t cur_byte = immediate_val & 255;
        if (reg2_ptr[i] < cur_byte + temp) {
          reg1_ptr[i] = reg2_ptr[i] + 256 - temp - cur_byte;
          temp = 1; // Increase carry
        } else {
          reg1_ptr[i] = reg2_ptr[i] - temp - cur_byte;
          temp = 0; // Carry has been used
        }
        immediate_val >>= 8;
      }
      SCMULATE_ERROR_IF(0, temp == 1, "Registers must be possitive numbers, addition of numbers resulted in negative number. Carry was 1 at the end of the operation");
    } else {
      // REGISTER REGISTER ADD CASE
      decoded_reg_t reg3 = instructions::decodeRegister(inst->getOp3());
      unsigned char * reg2_ptr = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number); 
      unsigned char * reg3_ptr = this->reg_file_m->getRegisterByName(reg3.reg_size, reg3.reg_number); 

      // Where to store the result
      unsigned char * reg1_ptr = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number); 
      int32_t size_reg_bytes = this->reg_file_m->getRegisterSizeInBytes(reg1.reg_size);

      // Subtraction
      uint32_t temp = 0;
      for (int32_t i = size_reg_bytes-1; i >= 0; --i) {
        if (reg2_ptr[i] < reg3_ptr[i] + temp) {
          reg1_ptr[i] = reg2_ptr[i] + 256 - temp - reg3_ptr[i];
          temp = 1; // Increase carry
        } else {
          reg1_ptr[i] = reg2_ptr[i] - temp - reg3_ptr[i];
          temp = 0; // Carry has been used
        }
      }
      SCMULATE_ERROR_IF(0, temp == 1, "Registers must be possitive numbers, addition of numbers resulted in negative number. Carry was 1 at the end of the operation");
    }
    return;
  }

  /////////////////////////////////////////////////////
  ///// ARITHMETIC LOGIC FOR THE SHFL INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "SHFL") {
    SCMULATE_ERROR(0, "THE SHFL OPERATION HAS NOT BEEN IMPLEMENTED. KILLING THIS")
    #pragma omp atomic write
    *(this->aliveSignal) = false;
  }

  /////////////////////////////////////////////////////
  ///// ARITHMETIC LOGIC FOR THE SHFR INSTRUCTION
  /////////////////////////////////////////////////////
  if (inst->getInstruction() == "SHFR") {
    SCMULATE_ERROR(0, "THE SHFR OPERATION HAS NOT BEEN IMPLEMENTED. KILLING THIS")
    #pragma omp atomic write
    *(this->aliveSignal) = false;
  }
}

void
scm::fetch_decode_module::assignExecuteInstruction(scm::decoded_instruction_t * inst) {
  // Counting the number of arguments 
  // TODO: THERE IS A BETTER WAY TO DO THIS, BUT THIS DO FOR NOW
  // We currently only support 3 arguments. Change this 
  unsigned char ** newArgs = new unsigned char*[3];
  if (inst->getOp1() != "") {
    decoded_reg_t reg1 = instructions::decodeRegister(inst->getOp1());
    newArgs[0] = this->reg_file_m->getRegisterByName(reg1.reg_size, reg1.reg_number); 
  } else {
    newArgs[0] = nullptr;
  }
  if (inst->getOp2() != "") {
    decoded_reg_t reg2 = instructions::decodeRegister(inst->getOp2());
    newArgs[1] = this->reg_file_m->getRegisterByName(reg2.reg_size, reg2.reg_number); 
  } else {
    newArgs[1] = nullptr;
  }
  if (inst->getOp3() != "") {
    decoded_reg_t reg3 = instructions::decodeRegister(inst->getOp3());
    newArgs[2] = this->reg_file_m->getRegisterByName(reg3.reg_size, reg3.reg_number); 
  } else {
    newArgs[2] = nullptr;
  }
  
  // TODO Support for immediate values? 
  scm::codelet * newCodelet = scm::codeletFactory::createCodelet(inst->getInstruction(), newArgs);
  this->ctrl_st_m->get_executor(0)->assign(newCodelet);
  while (!this->ctrl_st_m->get_executor(0)->is_empty());
}