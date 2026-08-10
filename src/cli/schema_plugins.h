#pragma once

#include "cli/schema.h"

#include <string_view>

namespace noctalia::plugins {

  inline constexpr cli_schema::CliPositional kLintPositionals[] = { //
      {.name = "path", .description = "plugin directory or directory of plugins", .required = false}
  };

  inline constexpr cli_schema::CliFlag kLintFlags[] = {
      {.longName = "--help", .description = "Print help for lint"},
  };

  inline constexpr cli_schema::CliCommand kLintCmd = {
      .name = "lint",
      .summary = "Cross-check plugin settings against getConfig() calls",
      .flags = kLintFlags,
      .positionals = kLintPositionals,
  };

  inline constexpr cli_schema::CliCommand kPluginsSubcommands[] = {kLintCmd};

  inline constexpr cli_schema::CliFlag kPluginsFlags[] = {
      {.longName = "--help", .description = "Print help for plugins"},
  };

  inline constexpr cli_schema::CliCommand kPluginsCmd = {
      .name = "plugins",
      .summary = "Offline tools for plugin authors",
      .flags = kPluginsFlags,
      .subcommands = kPluginsSubcommands
  };

} // namespace noctalia::plugins
