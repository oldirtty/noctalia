#include "ipc/cli.h"

#include "cli/parse.h"
#include "cli/schema_ipc.h"
#include "ipc/ipc_client.h"

#include <cstdio>
#include <nlohmann/json.hpp>
#include <print>
#include <string>
#include <string_view>

namespace noctalia::ipc {

  int runCli(int argc, char* argv[]) {
    if (argc < 3) {
      std::println(stderr, "error: msg requires a command (try: noctalia msg --help)");
      return 1;
    }

    const std::string_view subcmd = argv[2];

    if (subcmd == "--help") {
      if (IpcClient::send("--help") == 0) {
        return 0;
      }

      // Fallback
      std::println("Usage: noctalia msg <command> [args...]\n");
      std::println("Commands:");
      for (const auto& cmd : getIpcSubcommands()) {
        std::println("  {:<24} {}", cmd.name, cmd.summary);
      }
      return 0;
    }

    if (subcmd == "notification-show") {
      const auto parsed = cli_schema::parseArgs(argc, argv, 3, kNotificationCmd);
      if (!parsed) {
        std::println(stderr, "{}", parsed.error());
        std::println(stderr, "Run 'noctalia msg notification-show --help' for usage.");
        return 1;
      }

      if (parsed->helpRequested) {
        return 0;
      }

      // As of consumesRemaining = true, positionals[1] holds the full body space-separated!
      nlohmann::json payload = {
          {"summary", parsed->positionals[0]},
          {"body", parsed->positionals[1]},
      };

      return IpcClient::send(std::string(subcmd) + " " + payload.dump());
    }

    std::string cmd = argv[2];
    for (int i = 3; i < argc; ++i) {
      cmd += ' ';
      cmd += argv[i];
    }
    return IpcClient::send(cmd);
  }

} // namespace noctalia::ipc
