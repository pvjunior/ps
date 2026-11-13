#include "MacroOutput.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <type_traits>

namespace {

// ============================================================================
// FUNÇÕES ADAPTADORAS (ISOLAMENTO DE CAMPOS DO INTEGRANTE 1)
// ============================================================================

// [PONTO DE CONEXÃO - MacroTypes.h]
// Caso o Integrante 1 nomeie o campo de texto da linha em MacroLine de outra forma
// (ex: .text ou .rawLine), altere apenas esta linha dentro do adaptador.
inline const std::string& extractLineText(const MacroLine& line) {
#if !__has_include("MacroTypes.h")
    if (!line.text.empty()) {
        return line.text;
    }
#endif
    return line.originalText;
}

// [PONTO DE CONEXÃO - MacroTypes.h]
// Caso o Integrante 1 altere o nome do campo de operação em MacroLine,
// ajuste apenas esta linha dentro do adaptador.
inline const std::string& extractOperation(const MacroLine& line) {
    return line.operation;
}

// [PONTO DE CONEXÃO - MacroTypes.h]
// Caso o Integrante 1 adicione níveis de severidade aos diagnósticos em MacroExpansionResult,
// ajuste apenas esta função adaptadora.
inline bool hasFatalExpansionErrors(const MacroExpansionResult& result) {
    return !result.diagnostics.empty();
}

// ============================================================================
// FUNÇÕES AUXILIARES DE FORMATAÇÃO E FILTRAGEM DE DIRETIVAS DE MACRO
// ============================================================================

std::string toUpperAscii(const std::string& input) {
    std::string out = input;
    for (char& ch : out) {
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
    }
    return out;
}

bool isWhitespace(char ch) {
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

// Extrai até os dois primeiros tokens antes de comentário ('.') para detectar MCDEFN / MCEND
// mesmo quando a linha tiver sido preenchida apenas com texto bruto.
std::vector<std::string> extractLeadingTokens(const std::string& lineText) {
    std::vector<std::string> tokens;
    std::size_t pos = 0;
    const std::size_t len = lineText.size();

    while (pos < len && isWhitespace(lineText[pos])) {
        ++pos;
    }
    if (pos >= len || lineText[pos] == '.') {
        return tokens;
    }

    bool inQuote = false;
    while (pos < len && tokens.size() < 2U) {
        while (pos < len && isWhitespace(lineText[pos])) {
            ++pos;
        }
        if (pos >= len || (!inQuote && lineText[pos] == '.')) {
            break;
        }
        const std::size_t start = pos;
        while (pos < len) {
            if (lineText[pos] == '\'') {
                inQuote = !inQuote;
            } else if (!inQuote && (isWhitespace(lineText[pos]) || lineText[pos] == '.')) {
                break;
            }
            ++pos;
        }
        if (pos > start) {
            tokens.push_back(toUpperAscii(lineText.substr(start, pos - start)));
        }
    }
    return tokens;
}

bool matchesMacroDirective(const MacroLine& line, const std::string& directiveUpper) {
    const std::string opUpper = toUpperAscii(extractOperation(line));
    if (opUpper == directiveUpper) {
        return true;
    }

    const std::vector<std::string> tokens = extractLeadingTokens(extractLineText(line));
    for (const std::string& token : tokens) {
        if (token == directiveUpper) {
            return true;
        }
    }
    return false;
}

template <typename T, typename = void>
struct HasLabelField : std::false_type {};
template <typename T>
struct HasLabelField<T, std::void_t<decltype(std::declval<const T&>().label)>>
    : std::true_type {};

template <typename T, typename = void>
struct HasOperandsField : std::false_type {};
template <typename T>
struct HasOperandsField<T, std::void_t<decltype(std::declval<const T&>().operands)>>
    : std::true_type {};

template <typename T, typename = void>
struct HasOperandField : std::false_type {};
template <typename T>
struct HasOperandField<T, std::void_t<decltype(std::declval<const T&>().operand)>>
    : std::true_type {};

template <typename T, typename = void>
struct HasCommentField : std::false_type {};
template <typename T>
struct HasCommentField<T, std::void_t<decltype(std::declval<const T&>().comment)>>
    : std::true_type {};

inline std::string extractOptionalString(const std::string& value) {
    return value;
}

inline std::string extractOptionalString(const std::optional<std::string>& value) {
    return value.has_value() ? *value : std::string{};
}

// Caso uma MacroLine tenha sido construída em memória apenas com os campos estruturados
// (sem preencher o campo de texto bruto), reconstrói a linha respeitando a sintaxe SIC/XE.
template <typename LineT = MacroLine>
std::string synthesizeLineFromStructuredFields(const LineT& line) {
    std::string labelStr;
    if constexpr (HasLabelField<LineT>::value) {
        labelStr = extractOptionalString(line.label);
    }

    const std::string& opStr = extractOperation(line);

    std::string operandStr;
    if constexpr (HasOperandsField<LineT>::value) {
        if constexpr (std::is_convertible_v<decltype(line.operands), std::string>) {
            operandStr = line.operands;
        }
    }
    if (operandStr.empty()) {
        if constexpr (HasOperandField<LineT>::value) {
            if constexpr (std::is_convertible_v<decltype(line.operand), std::string>) {
                operandStr = line.operand;
            }
        }
    }

    std::string commentStr;
    if constexpr (HasCommentField<LineT>::value) {
        commentStr = extractOptionalString(line.comment);
    }

    if (labelStr.empty() && opStr.empty() && operandStr.empty()) {
        return commentStr;
    }

    std::ostringstream oss;
    if (!labelStr.empty()) {
        oss << labelStr;
        if (!opStr.empty() || !operandStr.empty() || !commentStr.empty()) {
            oss << " ";
        }
    } else if (!opStr.empty()) {
        // Sem rótulo: indenta a operação para que o SourceParser do Checkpoint 2
        // não interprete o mnemônico na coluna 0 como se fosse um rótulo.
        oss << "      ";
    }

    if (!opStr.empty()) {
        oss << opStr;
        if (!operandStr.empty()) {
            oss << " " << operandStr;
        }
    }

    if (!commentStr.empty()) {
        if (!opStr.empty() || !labelStr.empty()) {
            oss << " ";
        }
        oss << commentStr;
    }

    return oss.str();
}

std::string sanitizeSingleLine(const std::string& raw) {
    std::string cleaned = raw;
    while (!cleaned.empty() && (cleaned.back() == '\r' || cleaned.back() == '\n')) {
        cleaned.pop_back();
    }
    return cleaned;
}

MacroDiagnostic makeOutputDiagnostic(const std::string& path, const std::string& message) {
    MacroDiagnostic diag;
    diag.where.file = path;
    diag.where.line = 0;
    diag.message = message;
    return diag;
}

} // namespace

MacroWriteResult writeExpandedSource(
    const MacroExpansionResult& result,
    const std::string& path) {
    MacroWriteResult writeResult;
    writeResult.outputPath = path;

    // 1. Bloqueio em caso de erro de expansão: preserva qualquer MASMAPRG.ASM anterior intacto.
    if (hasFatalExpansionErrors(result)) {
        writeResult.errorMessage =
            "expansao de macros contem erros fatais; gravacao de " + path + " cancelada";
        writeResult.diagnostics = result.diagnostics;
        return writeResult;
    }

    if (path.empty()) {
        writeResult.errorMessage = "caminho de saida expandida vazio";
        writeResult.diagnostics.push_back(
            makeOutputDiagnostic("<saida>", writeResult.errorMessage));
        return writeResult;
    }

    // 2. Filtragem de diretivas de macro (MCDEFN, MCEND e corpos de definições não expandidas)
    //    e formatação preservando a ordem exata das linhas.
    std::ostringstream buffer;
    std::size_t writtenCount = 0;
    int unexpandedDefinitionDepth = 0;

    for (const MacroLine& line : result.expandedLines) {
        if (matchesMacroDirective(line, "MCDEFN")) {
            ++unexpandedDefinitionDepth;
            continue;
        }
        if (matchesMacroDirective(line, "MCEND")) {
            if (unexpandedDefinitionDepth > 0) {
                --unexpandedDefinitionDepth;
            }
            continue;
        }
        if (unexpandedDefinitionDepth > 0) {
            continue;
        }

        const std::string& rawText = extractLineText(line);
        const std::string chosenText =
            !rawText.empty() ? rawText : synthesizeLineFromStructuredFields(line);

        buffer << sanitizeSingleLine(chosenText) << '\n';
        ++writtenCount;
    }

    std::string finalContent = buffer.str();
    if (finalContent.empty() || finalContent.back() != '\n') {
        finalContent.push_back('\n');
    }

    // 3. Escrita atômica e segura utilizando arquivo temporário (path + ".tmp").
    const std::filesystem::path targetPath(path);
    std::filesystem::path tempPath(path + ".tmp");

    {
        std::ofstream stream(tempPath, std::ios::binary | std::ios::trunc);
        if (!stream.is_open() || !stream.good()) {
            writeResult.errorMessage =
                "nao foi possivel abrir o arquivo temporario para escrita: " + tempPath.string();
            writeResult.diagnostics.push_back(
                makeOutputDiagnostic(path, writeResult.errorMessage));
            return writeResult;
        }

        stream.write(finalContent.data(), static_cast<std::streamsize>(finalContent.size()));
        stream.flush();
        const bool flushOk = stream.good();
        stream.close();

        if (!flushOk || stream.fail() || stream.bad()) {
            std::error_code removeEc;
            std::filesystem::remove(tempPath, removeEc);
            writeResult.errorMessage =
                "falha de I/O ao gravar o codigo expandido em: " + tempPath.string();
            writeResult.diagnostics.push_back(
                makeOutputDiagnostic(path, writeResult.errorMessage));
            return writeResult;
        }
    }

    // 4. Substituição atômica do arquivo final somente após confirmação de escrita completa.
    std::error_code renameEc;
    std::filesystem::rename(tempPath, targetPath, renameEc);
    if (renameEc) {
        // Tratamento para plataformas/sistemas de arquivos onde rename exige remoção prévia do destino.
        std::error_code existsEc;
        if (std::filesystem::exists(targetPath, existsEc) && !existsEc) {
            std::error_code removeTargetEc;
            std::filesystem::remove(targetPath, removeTargetEc);
            if (!removeTargetEc) {
                renameEc.clear();
                std::filesystem::rename(tempPath, targetPath, renameEc);
            }
        }
    }

    if (renameEc) {
        std::error_code cleanupEc;
        std::filesystem::remove(tempPath, cleanupEc);
        writeResult.errorMessage =
            "nao foi possivel substituir o arquivo final " + path + ": " + renameEc.message();
        writeResult.diagnostics.push_back(
            makeOutputDiagnostic(path, writeResult.errorMessage));
        return writeResult;
    }

    writeResult.ok = true;
    writeResult.written = true;
    writeResult.linesWritten = writtenCount;
    return writeResult;
}
