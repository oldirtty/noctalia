#include "cli/parse.h"

#include <cstdio>
#include <cstring>
#include <print>

namespace noctalia::cli_schema {
  namespace {

    const CliFlag* findFlag(const CliCommand& cmd, std::string_view arg) {
      for (const auto& flag : cmd.flags) {
        if (arg == flag.longName || (!flag.shortName.empty() && arg == flag.shortName)) {
          return &flag;
        }
      }
      return nullptr;
    }

    bool matchesChoice(const CliPositional& pos, std::string_view value) {
      if (pos.choices.empty()) {
        return true; // free-form
      }
      for (const auto& choice : pos.choices) {
        if (value == choice) {
          return true;
        }
      }
      return false;
    }

    std::string buildChoicesError(const CliPositional& pos) {
      std::string err = "error: expected ";
      for (std::size_t i = 0; i < pos.choices.size(); ++i) {
        if (i > 0 && i == pos.choices.size() - 1)
          err += " or ";
        else if (i > 0)
          err += ", ";
        err += pos.choices[i];
      }
      err += " for <" + std::string(pos.name) + ">";
      return err;
    }

  } // namespace

  std::expected<ParsedArgs, std::string> parseArgs(int argc, char* argv[], int startIndex, const CliCommand& cmd) {
    ParsedArgs result;
    std::size_t nextPositional = 0;

    for (int i = startIndex; i < argc; ++i) {
      const std::string_view arg = argv[i];

      if (arg == "--help") {
        std::println("{}", cmd.helpText);
        result.helpRequested = true;
        return result;
      }

      if (arg.starts_with('-')) {
        const CliFlag* flag = findFlag(cmd, arg);
        if (flag == nullptr) {
          return std::unexpected(std::string("error: unknown argument: ") + std::string(arg));
        }
        if (flag->takesValue) {
          if (i + 1 >= argc) {
            return std::unexpected("error: " + std::string(flag->longName) + " requires a value");
          }
          result.flags[flag->longName] = argv[++i];
        } else {
          result.flags[flag->longName] = "1"; // presence marker
        }
        continue;
      }

      if (nextPositional < cmd.positionals.size()) {
        const CliPositional& pos = cmd.positionals[nextPositional];
        if (!matchesChoice(pos, arg)) {
          return std::unexpected(buildChoicesError(pos));
        }

        if (pos.consumesRemaining) {
          std::string combined = std::string(arg);
          for (int j = i + 1; j < argc; ++j) {
            combined += " ";
            combined += argv[j];
          }
          result.positionals.emplace_back(combined);
          ++nextPositional;
          break; // consumed all remaining args
        } else {
          result.positionals.emplace_back(arg);
          ++nextPositional;
          continue;
        }
      }

      // Generate dynamic usage string if too many arguments
      std::string extraErr = "error: usage: " + std::string(cmd.name);
      for (const auto& pos : cmd.positionals) {
        if (pos.required) {
          extraErr += " <" + std::string(pos.name) + ">";
        } else {
          extraErr += " [" + std::string(pos.name) + "]";
        }
      }
      return std::unexpected(extraErr + " (unexpected argument: " + std::string(arg) + ")");
    }

    for (std::size_t i = 0; i < cmd.positionals.size(); ++i) {
      if (cmd.positionals[i].required && i >= result.positionals.size()) {
        return std::unexpected("error: missing required argument <" + std::string(cmd.positionals[i].name) + ">");
      }
    }
    for (const auto& flag : cmd.flags) {
      if (flag.required && !result.hasFlag(flag.longName)) {
        return std::unexpected("error: missing required flag " + std::string(flag.longName));
      }
    }

    return result;
  }

} // namespace noctalia::cli_schema
