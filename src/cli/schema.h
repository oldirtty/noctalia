#pragma once

#include <span>
#include <string_view>

namespace noctalia::cli_schema {

  // A single named flag. `--flag <value>` if takesValue is true, `--flag` (boolean) otherwise.
  struct CliFlag {
    std::string_view longName;  // "--target"
    std::string_view shortName; // "" if none
    std::string_view description;
    bool takesValue = false;
    bool required = false;
  };

  // A positional argument. `choices` empty means free-form (e.g. a file path);
  // non-empty means the value must be one of these (e.g. export's merged|full).
  struct CliPositional {
    std::string_view name; // shown in usage, e.g. "path" or "mode"
    std::string_view description;
    std::span<const std::string_view> choices; // empty = free-form
    bool required = false;
    bool consumesRemaining = false; // if true, absorbs all remaining arguments as a single space-separated string
  };

  // One CLI command (a leaf like `validate`, or a group like `config` itself).
  // `helpText` is the full pre-rendered --help body for this exact command.
  struct CliCommand {
    std::string_view name;
    std::string_view summary; // one-line, used in the parent's command list
    std::string_view helpText;
    std::span<const CliFlag> flags;
    std::span<const CliPositional> positionals;
    std::span<const CliCommand> subcommands;
  };

} // namespace noctalia::cli_schema