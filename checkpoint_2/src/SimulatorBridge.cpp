#include "SimulatorBridge.h"

SimulatorBridge::SimulatorBridge(const ObjectProgram& prog) : program(prog) {}

std::vector<Diagnostic> SimulatorBridge::loadInto(Machine& machine) const {
    std::vector<Diagnostic> diagnostics;

    // Get all continuous blocks of code from the ObjectProgram
    const std::vector<ObjectBlock>& blocks = program.getBlocks();

    // Iterate through each block and write it to the simulator
    for (const auto& block : blocks) {
        
        // machine.loader().write() injects bytes without altering the machine's execution state
        LoadResult result = machine.loader().write(block.address, block.bytes);

        // If the simulator rejects the bytes (e.g., address exceeds memory size), log a diagnostic
        if (!result.ok) {
            Diagnostic diag;
            diag.where.file = "SimulatorBridge"; // Identifier for the source of the error
            diag.where.line = 0;                 // No specific source code line applies here
            diag.message = "Failed to load block at address 0x" + std::to_string(block.address) + 
                           ". Reason: " + result.error;
            
            diagnostics.push_back(diag);
        }
    }

    return diagnostics;
}