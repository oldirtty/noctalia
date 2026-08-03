#include "completions/cli.h"

#include "cli/parse.h"
#include "cli/schema_root.h"
#include "completions/generator.h"

#include <print>
#include <string_view>

namespace noctalia::completions {

  int runCli(int argc, char* argv[]) {
    if (argc >= 3 && (std::string_view(argv[2]) == "--help" || std::string_view(argv[2]) == "-h")) {
      std::println("Usage: noctalia completions [bash|fish|zsh]\n");
      std::println("Print shell completion script dynamically generated from CLI schema.");
      return 0;
    }

    if (argc < 3) {
      std::println(stderr, "error: missing shell argument (expected bash, fish, or zsh)");
      std::println(stderr, "Run 'noctalia completions --help' for usage.");
      return 1;
    }

    const std::string_view shell = argv[2];
    std::string script;

    if (shell == "fish") {
      script = generateFish(cli::kRootCmd);
    } else if (shell == "zsh") {
      script = generateZsh(cli::kRootCmd);
    } else if (shell == "bash") {
      script = generateBash(cli::kRootCmd);
    } else {
      std::println(stderr, "error: unknown shell '{}' (expected bash, fish, or zsh)", shell);
      return 1;
    }

    std::print("{}", script);
    return 0;
  }

} // namespace noctalia::completions
