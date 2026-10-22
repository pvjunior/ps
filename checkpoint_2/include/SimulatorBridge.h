#pragma once

#include <vector>
#include "ObjectProgram.h"
#include "AssemblerTypes.h"
#include "Machine.h" // Included from checkpoint_1/structures/include

// Class responsible for bridging the Assembler's output with the Simulator
class SimulatorBridge {
private:
    const ObjectProgram& program;

public:
    // Constructor receiving the parsed object program
    explicit SimulatorBridge(const ObjectProgram& prog);

    // Injects the object program blocks into the machine's memory.
    // Returns a list of diagnostics if errors occur (e.g., out-of-bounds memory).
    std::vector<Diagnostic> loadInto(Machine& machine) const;
};