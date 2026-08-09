#pragma once

#include "cli/schema.h"
#include "cli/schema_config.h"
#include "cli/schema_ipc.h"
#include "cli/schema_plugins.h"
#include "cli/schema_theme.h"

#include <array>
#include <string_view>

namespace noctalia::cli {

  inline constexpr std::array<std::string_view, 3> kShellChoices = {"bash", "fish", "zsh"};

  inline constexpr cli_schema::CliPositional kCompletionsPositionals[] = {
      {.name = "shell", .description = "Target shell", .choices = kShellChoices}
  };

  inline constexpr cli_schema::CliCommand kCompletionsCmd = {
      .name = "completions", .summary = "Generate shell completion scripts", .positionals = kCompletionsPositionals
  };

  inline constexpr cli_schema::CliCommand kRootSubcommands[] = {
      kCompletionsCmd, config::kConfigCmd, ipc::kMsgCmd, plugins::kPluginsCmd, theme::kThemeCmd,
  };

  inline constexpr cli_schema::CliCommand kRootCmd = {
      .name = "noctalia",
      .summary = "A sleek, customizable desktop shell crafted for Wayland",
      .subcommands = kRootSubcommands,
  };

} // namespace noctalia::cli
