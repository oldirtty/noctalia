#pragma once

#include "cli/schema.h"

#include <expected>
#include <string>
#include <unordered_map>
#include <vector>

namespace noctalia::cli_schema {

  struct ParsedArgs {
    // Flag values by longName, e.g. parsed.flags["--target"] == "/some/dir".
    // Boolean flags present in the map (any value) mean "set"; absent means unset.
    std::unordered_map<std::string_view, std::string> flags;
    // Positional values, in declaration order (parallel to CliCommand::positionals).
    std::vector<std::string> positionals;
    bool helpRequested = false;

    [[nodiscard]] bool hasFlag(std::string_view longName) const { return flags.contains(longName); }
    [[nodiscard]] std::string_view flagValue(std::string_view longName) const {
      const auto it = flags.find(longName);
      return it == flags.end() ? std::string_view{} : std::string_view{it->second};
    }
  };

  // Parses argv[startIndex..argc) against `cmd`'s flags/positionals.
  [[nodiscard]] std::expected<ParsedArgs, std::string>
  parseArgs(int argc, char* argv[], int startIndex, const CliCommand& cmd);

} // namespace noctalia::cli_schema
