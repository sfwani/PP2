#ifndef GRITVM_H
#define GRITVM_H

#include "GritVMBase.hpp"
#include <list>
#include <vector>
#include <string>
#include <fstream>

class GritVM : public GritVMInterface {
private:
    std::vector<long> dataMem;                     // Holds data values
    std::list<Instruction> instructMem;            // Holds instructions
    std::list<Instruction>::iterator currentInstruct; 
    STATUS machineStatus;                          // Current status (WAITING, RUNNING, HALTED, etc.)
    long accumulator;                              // For arithmetic operations

    // Evaluate the current instruction and decide how many steps to move
    long evaluate(const Instruction& inst);

    // Move the instruction pointer by jumpDistance
    void advance(long jumpDistance);

    // Check if memory access is valid
    bool validateMemoryAccess(long location) const;

    // Handle operations with constants
    long handleConstOperation(INSTRUCTION_SET op, long constant);

    // Handle operations with memory locations
    long handleMemOperation(INSTRUCTION_SET op, long memLocation);

    // Handle jump instructions
    long handleJump(INSTRUCTION_SET op, long distance);

public:
    // Constructor sets machine to WAITING
    GritVM();

    // Load GVM program from a file and initialize data memory
    STATUS load(const std::string filename, const std::vector<long>& initialMemory) override;

    // Run the loaded program
    STATUS run() override;

    // Return current data memory contents
    std::vector<long> getDataMem() override;

    // Reset machine state
    STATUS reset() override;

    // Print machine state for debugging
    void printVM(bool printData = true, bool printInstruction = true) const;

    // Destructor
    ~GritVM() = default;

    // Prevent copying
    GritVM(const GritVM&) = delete;
    GritVM& operator=(const GritVM&) = delete;
};

#endif // GRITVM_H
