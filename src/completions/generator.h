#pragma once

#include "cli/schema.h"

#include <string>

namespace noctalia::completions {
  std::string generateFish(const cli_schema::CliCommand& root);
  std::string generateZsh(const cli_schema::CliCommand& root);
  std::string generateBash(const cli_schema::CliCommand& root);
} // namespace noctalia::completions
