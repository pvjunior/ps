#include "ObjectParser.h"

#include <charconv>
#include <fstream>
#include <sstream>
#include <string_view>
#include <utility>

namespace {

bool parseHex(std::string_view field, std::uint32_t& value) {
    const char* end = field.data() + field.size();
    const auto result = std::from_chars(field.data(), end, value, 16);
    return result.ec == std::errc{} && result.ptr == end;
}

bool parseHeader(const std::string& line, HeaderRecord& record) {
    if (line.size() != 19) return false;

    const std::string_view fields(line);
    record.programName = fields.substr(1, 6);
    const auto last = record.programName.find_last_not_of(' ');
    record.programName.resize(last == std::string::npos ? 0 : last + 1);
    return parseHex(fields.substr(7, 6), record.originalStartAddress) &&
           parseHex(fields.substr(13, 6), record.programLength);
}

bool parseText(const std::string& line, TextRecord& record) {
    if (line.size() < 9) return false;

    const std::string_view fields(line);
    std::uint32_t count = 0;
    if (!parseHex(fields.substr(1, 6), record.originalAddress) ||
        !parseHex(fields.substr(7, 2), count) ||
        count == 0 || count > 30 || line.size() != 9 + 2 * count) {
        return false;
    }

    record.bytes.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint32_t byte = 0;
        if (!parseHex(fields.substr(9 + 2 * i, 2), byte)) return false;
        record.bytes.push_back(static_cast<std::uint8_t>(byte));
    }
    return true;
}

bool parseModification(const std::string& line, ModificationRecord& record) {
    if (line.size() != 9) return false;

    const std::string_view fields(line);
    std::uint32_t width = 0;
    if (!parseHex(fields.substr(1, 6), record.originalAddress) ||
        !parseHex(fields.substr(7, 2), width) || width == 0) {
        return false;
    }
    record.fieldLengthHalfBytes = static_cast<std::uint8_t>(width);
    return true;
}

bool parseEnd(const std::string& line, EndRecord& record) {
    // The shared contract allows an end record without an entry address.
    if (line.size() == 1) return true;
    if (line.size() != 7) return false;

    std::uint32_t address = 0;
    if (!parseHex(std::string_view(line).substr(1, 6), address)) return false;
    record.originalExecutionAddress = address;
    return true;
}

} // namespace

ObjectParseResult parseObjectText(const std::string& text,
                                 const std::string& sourceName) {
    ObjectParseResult result;
    ParsedObject object;
    object.location = {sourceName, 0};
    object.originalText = text;

    std::istringstream input(text);
    std::string line;
    std::uint32_t lineNumber = 0;
    bool sawHeader = false;
    bool sawModification = false;
    bool sawEnd = false;

    while (std::getline(input, line)) {
        const LoaderSourceLocation where{sourceName, ++lineNumber};
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        const auto diagnose = [&](const char* message) {
            result.diagnostics.push_back({where, message, line});
        };

        if (sawEnd) {
            diagnose("record after E");
            continue;
        }
        if (!sawHeader && line.front() != 'H') {
            diagnose("first record must be H");
            continue;
        }

        switch (line.front()) {
            case 'H': {
                if (sawHeader) {
                    diagnose("duplicate H record");
                    break;
                }
                sawHeader = true;
                HeaderRecord record;
                record.location = where;
                record.originalText = line;
                if (parseHeader(line, record)) object.header = std::move(record);
                else diagnose("invalid H record");
                break;
            }
            case 'T': {
                if (sawModification) {
                    diagnose("T record after M");
                    break;
                }
                TextRecord record;
                record.location = where;
                record.originalText = line;
                if (parseText(line, record)) object.textRecords.push_back(std::move(record));
                else diagnose("invalid T record: check address, byte count and hex data");
                break;
            }
            case 'M': {
                sawModification = true;
                ModificationRecord record;
                record.location = where;
                record.originalText = line;
                if (parseModification(line, record)) {
                    object.modificationRecords.push_back(std::move(record));
                } else {
                    diagnose("invalid M record: check address and half-byte count");
                }
                break;
            }
            case 'E': {
                sawEnd = true;
                EndRecord record;
                record.location = where;
                record.originalText = line;
                if (parseEnd(line, record)) object.end = std::move(record);
                else diagnose("invalid E record");
                break;
            }
            default:
                diagnose("unknown object record");
        }
    }

    if (!sawHeader) result.diagnostics.push_back({{sourceName, 0}, "missing H record", {}});
    if (!sawEnd) result.diagnostics.push_back({{sourceName, 0}, "missing E record", {}});
    if (result.diagnostics.empty()) result.object = std::move(object);
    return result;
}

ObjectParseResult parseObjectFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {std::nullopt, {{{path, 0}, "could not open object file", {}}}};

    std::ostringstream contents;
    contents << input.rdbuf();
    if (input.bad() || contents.bad()) {
        return {std::nullopt, {{{path, 0}, "could not read object file", {}}}};
    }

    return parseObjectText(contents.str(), path);
}
