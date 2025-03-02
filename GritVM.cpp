#include "GritVM.hpp"
#include <iostream>
#include <sstream>

// Constructor
GritVM::GritVM() {
    reset();
}

// Reset the VM
STATUS GritVM::reset() {
    accumulator = 0;
    dataMem.clear();
    instructMem.clear();
    machineStatus = WAITING;
    return machineStatus;
}

// Check valid memory access
bool GritVM::validateMemoryAccess(long location) const {
    return (location >= 0 && static_cast<size_t>(location) < dataMem.size());
}

// Load instructions from file and set initial memory
STATUS GritVM::load(const std::string filename, const std::vector<long>& initialMemory) {
    if (machineStatus != WAITING) {
        return machineStatus;
    }

    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    instructMem.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        Instruction inst = GVMHelper::parseInstruction(line);
        if (inst.operation == UNKNOWN_INSTRUCTION) {
            machineStatus = ERRORED;
            return machineStatus;
        }
        instructMem.push_back(inst);
    }

    dataMem = initialMemory;
    machineStatus = instructMem.empty() ? WAITING : READY;
    if (!instructMem.empty()) {
        currentInstruct = instructMem.begin();
    }

    return machineStatus;
}

// Run the loaded program
STATUS GritVM::run() {
    if (machineStatus != READY) {
        return machineStatus;
    }

    machineStatus = RUNNING;
    currentInstruct = instructMem.begin();

    while (machineStatus == RUNNING) {
        long jumpDistance = evaluate(*currentInstruct);
        if (machineStatus == RUNNING) {
            advance(jumpDistance);
        }
    }
    return machineStatus;
}

// Handle constant operations
long GritVM::handleConstOperation(INSTRUCTION_SET operation, long constant) {
    switch (operation) {
        case ADDCONST:
            accumulator += constant;
            break;
        case SUBCONST:
            accumulator -= constant;
            break;
        case MULCONST:
            accumulator *= constant;
            break;
        case DIVCONST:
            if (constant == 0) {
                machineStatus = ERRORED;
                return 1;
            }
            accumulator /= constant;
            break;
        default:
            machineStatus = ERRORED;
            break;
    }
    return 1;
}

// Handle memory operations
long GritVM::handleMemOperation(INSTRUCTION_SET operation, long memLocation) {
    if (!validateMemoryAccess(memLocation)) {
        machineStatus = ERRORED;
        return 1;
    }
    switch (operation) {
        case ADDMEM:
            accumulator += dataMem[memLocation];
            break;
        case SUBMEM:
            accumulator -= dataMem[memLocation];
            break;
        case MULMEM:
            accumulator *= dataMem[memLocation];
            break;
        case DIVMEM:
            if (dataMem[memLocation] == 0) {
                machineStatus = ERRORED;
                return 1;
            }
            accumulator /= dataMem[memLocation];
            break;
        default:
            machineStatus = ERRORED;
            break;
    }
    return 1;
}

// Handle jumps
long GritVM::handleJump(INSTRUCTION_SET operation, long distance) {
    if (distance == 0) {
        machineStatus = ERRORED;
        return 1;
    }
    switch (operation) {
        case JUMPREL:
            return distance;
        case JUMPZERO:
            return (accumulator == 0) ? distance : 1;
        case JUMPNZERO:
            return (accumulator != 0) ? distance : 1;
        default:
            machineStatus = ERRORED;
            return 1;
    }
}

// Evaluate an instruction
long GritVM::evaluate(const Instruction& inst) {
    switch (inst.operation) {
        case CLEAR:
            accumulator = 0;
            return 1;
        case AT:
            if (!validateMemoryAccess(inst.argument)) {
                machineStatus = ERRORED;
                return 1;
            }
            accumulator = dataMem[inst.argument];
            return 1;
        case SET:
            if (!validateMemoryAccess(inst.argument)) {
                machineStatus = ERRORED;
                return 1;
            }
            dataMem[inst.argument] = accumulator;
            return 1;
        case INSERT:
            if (static_cast<size_t>(inst.argument) > dataMem.size()) {
                machineStatus = ERRORED;
                return 1;
            }
            dataMem.insert(dataMem.begin() + inst.argument, accumulator);
            return 1;
        case ERASE:
            if (!validateMemoryAccess(inst.argument)) {
                machineStatus = ERRORED;
                return 1;
            }
            dataMem.erase(dataMem.begin() + inst.argument);
            return 1;

        case ADDCONST: case SUBCONST: case MULCONST: case DIVCONST:
            return handleConstOperation(inst.operation, inst.argument);

        case ADDMEM: case SUBMEM: case MULMEM: case DIVMEM:
            return handleMemOperation(inst.operation, inst.argument);

        case JUMPREL: case JUMPZERO: case JUMPNZERO:
            return handleJump(inst.operation, inst.argument);

        case NOOP:
            return 1;
        case HALT:
            machineStatus = HALTED;
            return 1;
        case OUTPUT:
            std::cout << accumulator << std::endl;
            return 1;
        case CHECKMEM:
            if (dataMem.size() < static_cast<size_t>(inst.argument)) {
                machineStatus = ERRORED;
            }
            return 1;
        default:
            machineStatus = ERRORED;
            return 1;
    }
}

// Advance the instruction pointer
void GritVM::advance(long jumpDistance) {
    if (jumpDistance == 0) {
        machineStatus = ERRORED;
        return;
    }
    while (jumpDistance > 0 && currentInstruct != instructMem.end()) {
        ++currentInstruct;
        --jumpDistance;
    }
    while (jumpDistance < 0 && currentInstruct != instructMem.begin()) {
        --currentInstruct;
        ++jumpDistance;
    }
    if (currentInstruct == instructMem.end()) {
        machineStatus = HALTED;
    }
}

// Return current data memory
std::vector<long> GritVM::getDataMem() {
    return dataMem;
}

// Print VM state
void GritVM::printVM(bool printData, bool printInstruction) const {
    std::cout << "Status: " << GVMHelper::statusToString(machineStatus) << std::endl;
    std::cout << "Accumulator: " << accumulator << std::endl;

    if (printData) {
        std::cout << "*** Data Memory ***" << std::endl;
        for (size_t i = 0; i < dataMem.size(); ++i) {
            std::cout << "Location " << i << ": " << dataMem[i] << std::endl;
        }
    }
    if (printInstruction) {
        std::cout << "*** Instruction Memory ***" << std::endl;
        int index = 0;
        for (auto& inst : instructMem) {
            std::cout << "Instruction " << index++ << ": "
                      << GVMHelper::instructionToString(inst.operation)
                      << " " << inst.argument << std::endl;
        }
    }
}
