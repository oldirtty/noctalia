#include "completions/cli.h"

#include "cli/parse.h"
#include "cli/schema_root.h"
#include "completions/generator.h"

#include <print>
#include <string_view>

namespace noctalia::completions {
  namespace {

    using cli_schema::CliCommand;
    using cli_schema::CliFlag;
    using cli_schema::CliPositional;
    using cli_schema::ParsedArgs;

    constexpr const char* kHelpText = "Usage: noctalia completions [bash|fish|zsh]\n"
                                      "\n"
                                      "Print a shell completion script to stdout.\n"
                                      "\n"
                                      "Shells:\n"
                                      "  bash\n"
                                      "  zsh\n"
                                      "  fish\n"
                                      "\n"
                                      "Examples:\n"
                                      "  source <(noctalia completions bash)\n"
                                      "  noctalia completions fish > ~/.config/fish/completions/noctalia.fish\n";

    constexpr std::string_view kShellChoices[] = {"bash", "fish", "zsh"};

    constexpr CliPositional kCompletionsPositionals[] = {
        {.name = "shell",
         .description = "target shell (bash, fish, zsh)",
         .choices = kShellChoices,
         .required = true,
         .consumesRemaining = false},
    };

    constexpr CliCommand kCompletionsCmd = {
        .name = "completions",
        .summary = "Generate shell completions",
        .helpText = kHelpText,
        .flags = {},
        .positionals = kCompletionsPositionals,
    };
  } // namespace

  int runCli(int argc, char* argv[]) {
    const auto parsed = cli_schema::parseArgs(argc, argv, 2, kCompletionsCmd);
    if (!parsed) {
      std::println(stderr, "{}", parsed.error());
      std::println(stderr, "Run 'noctalia completions --help' for usage.");
      return 1;
    }

    if (parsed->helpRequested) {
      return 0;
    }

    const std::string_view shell = parsed->positionals[0];
    std::string script;

    if (shell == "fish") {
      script = generateFish(cli_schema::kRootCmd);
    } else if (shell == "zsh") {
      script = generateZsh(cli_schema::kRootCmd);
    } else if (shell == "bash") {
      script = generateBash(cli_schema::kRootCmd);
    } else {
      // Unreachable due to choices validation, but safe fallback
      std::println(stderr, "error: unknown shell '{}'", shell);
      return 1;
    }

    std::print("{}", script);
    return 0;
  }

} // namespace noctalia::completions
