#include "cli/parse.h"
#include "cli/schema.h"

#include <cassert>
#include <string>
#include <string_view>
#include <vector>

namespace {

  using namespace noctalia::cli_schema;

  // Mock schema definitions for testing
  constexpr CliFlag kTestFlags[] = {
      {.longName = "--output", .shortName = "-o", .takesValue = true, .required = true},
      {.longName = "--verbose", .shortName = "-v", .takesValue = false, .required = false},
  };

  constexpr std::string_view kModeChoices[] = {"fast", "slow"};
  constexpr CliPositional kTestPositionals[] = {
      {.name = "mode", .choices = kModeChoices, .required = true},
      {.name = "message", .required = false, .consumesRemaining = true},
  };

  constexpr CliCommand kTestCmd = {
      .name = "test-cmd",
      .flags = kTestFlags,
      .positionals = kTestPositionals,
  };

  void testValidParsingAndConsumesRemaining() {
    const char* argv[] = {"noctalia", "test-cmd", "fast", "--output", "file.txt", "--verbose", "Hello", "World", "!"};
    int argc = 9;

    auto result = parseArgs(argc, const_cast<char**>(argv), 2, kTestCmd);

    assert(result.has_value());
    assert(result->positionals.size() == 2);
    assert(result->positionals[0] == "fast");
    // Proves that multiple words are absorbed correctly (fixes the notification-show regression)
    assert(result->positionals[1] == "Hello World !");

    assert(result->hasFlag("--output"));
    assert(result->flagValue("--output") == "file.txt");

    assert(result->hasFlag("--verbose"));
    assert(result->flagValue("--verbose") == "1");
    assert(!result->helpRequested);
  }

  void testMissingRequiredPositional() {
    const char* argv[] = {"noctalia", "test-cmd", "--output", "file.txt"};
    int argc = 4;

    auto result = parseArgs(argc, const_cast<char**>(argv), 2, kTestCmd);
    assert(!result.has_value());
    assert(result.error() == "error: missing required argument <mode>");
  }

  void testInvalidChoice() {
    const char* argv[] = {"noctalia", "test-cmd", "invalid-mode", "--output", "file.txt"};
    int argc = 5;

    auto result = parseArgs(argc, const_cast<char**>(argv), 2, kTestCmd);
    assert(!result.has_value());
    assert(result.error() == "error: expected fast or slow for <mode>");
  }

  void testMissingRequiredFlag() {
    const char* argv[] = {"noctalia", "test-cmd", "fast"};
    int argc = 3;

    auto result = parseArgs(argc, const_cast<char**>(argv), 2, kTestCmd);
    assert(!result.has_value());
    assert(result.error() == "error: missing required flag --output");
  }

  void testMissingFlagValue() {
    const char* argv[] = {"noctalia", "test-cmd", "fast", "--output"};
    int argc = 4;

    auto result = parseArgs(argc, const_cast<char**>(argv), 2, kTestCmd);
    assert(!result.has_value());
    assert(result.error() == "error: --output requires a value");
  }

  void testUnknownArgument() {
    const char* argv[] = {"noctalia", "test-cmd", "fast", "--output", "file.txt", "--unknown"};
    int argc = 6;

    auto result = parseArgs(argc, const_cast<char**>(argv), 2, kTestCmd);
    assert(!result.has_value());
    assert(result.error() == "error: unknown argument: --unknown");
  }

  void testHelpRequested() {
    const char* argv[] = {"noctalia", "test-cmd", "--help"};
    int argc = 3;

    auto result = parseArgs(argc, const_cast<char**>(argv), 2, kTestCmd);
    assert(result.has_value());
    assert(result->helpRequested == true);
  }

} // namespace

int main() {
  testValidParsingAndConsumesRemaining();
  testMissingRequiredPositional();
  testInvalidChoice();
  testMissingRequiredFlag();
  testMissingFlagValue();
  testUnknownArgument();
  testHelpRequested();
  return 0;
}
