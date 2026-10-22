#include "ObjectProgram.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

ObjectProgram::ObjectProgram(const std::string& progName, uint32_t start, uint32_t size, uint32_t execAddr)
    : name(progName), startAddress(start), programSize(size), executionAddress(execAddr) {}

void ObjectProgram::addBlock(const ObjectBlock& block) {
    if (!block.bytes.empty()) {
        blocks.push_back(block);
    }
}

void ObjectProgram::addModification(const ModificationRecord& mod) {
    modifications.push_back(mod);
}

const std::vector<ObjectBlock>& ObjectProgram::getBlocks() const {
    return blocks;
}

std::string ObjectProgram::toText() const {
    std::ostringstream out;

    // -------------------------------------------------------------
    // H RECORD: H + name (6 chars left-justified) + start address (6 hex) + size (6 hex)
    // -------------------------------------------------------------
    std::string formattedName = name;
    if (formattedName.length() > 6) {
        formattedName = formattedName.substr(0, 6); // Truncate if it's larger than 6
    }
    
    out << "H" 
        << std::left << std::setw(6) << std::setfill(' ') << formattedName
        << std::right << std::setw(6) << std::setfill('0') << std::hex << std::uppercase << startAddress
        << std::setw(6) << std::setfill('0') << programSize << "\n";

    // -------------------------------------------------------------
    // T RECORDS: Break by gaps and max 30 bytes
    // -------------------------------------------------------------
    for (const auto& block : blocks) {
        uint32_t currentAddress = block.address;
        size_t bytesLeft = block.bytes.size();
        size_t offset = 0;

        // As long as there are bytes in this block, keep slicing into chunks of max 30 bytes
        while (bytesLeft > 0) {
            // SIC/XE object rule: a T record cannot have more than 30 bytes (0x1E)
            size_t chunkSize = std::min(bytesLeft, static_cast<size_t>(30));

            out << "T"
                << std::setw(6) << std::setfill('0') << std::hex << std::uppercase << currentAddress
                << std::setw(2) << std::setfill('0') << chunkSize;

            // Write the bytes in hexadecimal
            for (size_t i = 0; i < chunkSize; ++i) {
                out << std::setw(2) << std::setfill('0') << static_cast<int>(block.bytes[offset + i]);
            }
            out << "\n";

            // Update pointers for the next T line (if there are still bytes left in the current block)
            currentAddress += chunkSize;
            offset += chunkSize;
            bytesLeft -= chunkSize;
        }
    }

    // -------------------------------------------------------------
    // M RECORDS: Modifications (for format 4 addresses)
    // -------------------------------------------------------------
    for (const auto& mod : modifications) {
        out << "M"
            << std::setw(6) << std::setfill('0') << std::hex << std::uppercase << mod.address
            << std::setw(2) << std::setfill('0') << static_cast<int>(mod.halfBytes) << "\n";
    }

    // -------------------------------------------------------------
    // E RECORD: Execution address to start the program
    // -------------------------------------------------------------
    out << "E" << std::setw(6) << std::setfill('0') << std::hex << std::uppercase << executionAddress << "\n";

    return out.str();
}