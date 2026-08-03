#include "completions/generator.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>

namespace noctalia::completions {

  namespace {

    std::string escapeQuotes(std::string_view sv) {
      std::string res;
      res.reserve(sv.size() + 5);
      for (char c : sv) {
        if (c == '\'') {
          res += "\\'";
        } else if (c == '[') {
          res += "\\[";
        } else if (c == ']') {
          res += "\\]";
        } else {
          res += c;
        }
      }
      return res;
    }

    std::string toZshVarNamePart(std::string_view name) {
      std::string result(name);
      std::ranges::transform(result, result.begin(), [](unsigned char c) {
        return c == '-' ? '_' : static_cast<char>(std::toupper(c));
      });
      return result;
    }

    void generateFishCommand(std::ostringstream& ss, const cli_schema::CliCommand& cmd, const std::string& cmdPath) {
      std::string condition = std::format("__fish_noctalia_using_command {}", cmdPath);

      for (const auto& sub : cmd.subcommands) {
        ss << std::format(
            "complete -c noctalia -n '{}' -a '{}' -d '{}'\n", condition, sub.name, escapeQuotes(sub.summary)
        );

        std::string subPath = cmdPath + " " + std::string(sub.name);
        generateFishCommand(ss, sub, subPath);
      }

      for (const auto& flag : cmd.flags) {
        std::string flagOpt;
        if (!flag.shortName.empty()) {
          std::string_view shortTrimmed = flag.shortName.starts_with('-') ? flag.shortName.substr(1) : flag.shortName;
          flagOpt += std::format("-s {} ", shortTrimmed);
        }
        std::string_view longTrimmed = flag.longName.starts_with("--") ? flag.longName.substr(2) : flag.longName;
        flagOpt += std::format("-l '{}'", longTrimmed);

        if (flag.takesValue) {
          flagOpt += " -r";
        }

        ss << std::format(
            "complete -c noctalia -n '{}' {} -d '{}'\n", condition, flagOpt, escapeQuotes(flag.description)
        );
      }

      for (const auto& pos : cmd.positionals) {
        if (!pos.choices.empty()) {
          std::string choicesStr;
          for (auto choice : pos.choices) {
            std::string choiceStr(choice);
            if (choiceStr.find(' ') != std::string::npos) {
              choicesStr += std::format("\"{}\" ", choiceStr);
            } else {
              choicesStr += choiceStr + " ";
            }
          }
          if (!choicesStr.empty()) {
            choicesStr.pop_back();
          }
          ss << std::format("complete -c noctalia -n '{}' -a '{}'\n", condition, choicesStr);
        }
      }
    }

    std::string buildArgumentsItems(const cli_schema::CliCommand& cmd) {
      std::ostringstream items;
      items << "'(-h --help)'{-h,--help}'[Show help message]'";

      for (const auto& flag : cmd.flags) {
        if (flag.longName == "--help") {
          continue;
        }
        items << " \\\n                        '";
        if (!flag.shortName.empty()) {
          items
              << "("
              << flag.shortName
              << " "
              << flag.longName
              << ")'{"
              << flag.shortName
              << ","
              << flag.longName
              << "}'["
              << escapeQuotes(flag.description)
              << "]'";
        } else {
          items << flag.longName << "[" << escapeQuotes(flag.description) << "]'";
        }
        if (flag.takesValue) {
          items.seekp(-1, std::ios_base::end);
          items << ":" << flag.longName.substr(flag.longName.find_first_not_of('-')) << ":_files'";
        }
      }

      for (std::size_t i = 0; i < cmd.positionals.size(); ++i) {
        const auto& pos = cmd.positionals[i];
        const std::string indexSpec = std::to_string(i + 1);
        items << " \\\n                    '";
        if (!pos.choices.empty()) {
          items << indexSpec << ": :(";
          for (const auto& choice : pos.choices) {
            std::string choiceStr(choice);
            if (choiceStr.find(' ') != std::string::npos) {
              items << "'\\''" << choiceStr << "'\\'' ";
            } else {
              items << choiceStr << " ";
            }
          }
          items.seekp(-1, std::ios_base::end);
          items << ")'";
        } else {
          items << indexSpec << ": :_files'";
        }
      }

      items << " \\\n                    '*: :_files'";

      return items.str();
    }

    void generateZshCommandsRecursive(
        std::ostringstream& ss, const cli_schema::CliCommand& cmd, const std::string& funcName
    ) {
      if (cmd.subcommands.empty()) {
        return;
      }

      ss << std::format("_{}() {{\n", funcName);
      ss
          << "    local context state state_descr line\n"
          << "    typeset -A opt_args\n\n"
          << "    _arguments -C \\\n"
          << "        '(-h --help)'{-h,--help}'[Show help message]' \\\n"
          << "        '1: :->subcmds' \\\n"
          << "        '*:: :->subargs'\n\n"
          << "    case \"$state\" in\n"
          << "        subcmds)\n"
          << std::format(
                 "            _describe -t commands '{} command' _NOCTALIA_{}_COMMANDS\n", cmd.name,
                 toZshVarNamePart(cmd.name)
             )
          << "            ;;\n"
          << "        subargs)\n"
          << "            case $line[1] in\n";

      for (const auto& sub : cmd.subcommands) {
        const std::string subFuncName = funcName + "_" + std::string(sub.name);
        ss << std::format("                {})\n", sub.name);
        if (!sub.subcommands.empty()) {
          ss << std::format("                    _{}\n", subFuncName);
        } else {
          ss << std::format("                    _arguments {}\n", buildArgumentsItems(sub));
        }
        ss << "                    ;;\n";
      }

      ss
          << "            esac\n"
          << "            ;;\n"
          << "    esac\n"
          << "}\n\n";

      for (const auto& sub : cmd.subcommands) {
        if (!sub.subcommands.empty()) {
          generateZshCommandsRecursive(ss, sub, funcName + "_" + std::string(sub.name));
        }
      }
    }

    void generateBashCases(
        std::ostringstream& flags_ss, std::ostringstream& cmds_ss, const cli_schema::CliCommand& cmd,
        const std::string& path
    ) {

      std::string flags = "--help -h";
      for (const auto& flag : cmd.flags) {
        flags += " " + std::string(flag.longName);
        if (!flag.shortName.empty()) {
          flags += " " + std::string(flag.shortName);
        }
      }
      flags_ss << std::format(
          "        \"{}\") COMPREPLY=( $(compgen -W \"{}\" -- \"$cur\") ); return 0 ;;\n", path, flags
      );

      std::string completions;
      bool use_newlines = false;

      if (!cmd.subcommands.empty()) {
        for (const auto& sub : cmd.subcommands) {
          if (!completions.empty())
            completions += " ";
          completions += std::string(sub.name);
        }
      } else if (!cmd.positionals.empty()) {
        for (const auto& pos : cmd.positionals) {
          if (!pos.choices.empty()) {
            for (const auto& choice : pos.choices) {
              std::string c(choice);
              if (!completions.empty())
                completions += "\n";
              completions += c;
              if (c.find(' ') != std::string::npos)
                use_newlines = true;
            }
            use_newlines = true;
            break;
          }
        }
      }

      if (!completions.empty()) {
        if (use_newlines) {
          cmds_ss
              << std::format("        \"{}\")\n", path)
              << "            local IFS=$'\\n'\n"
              << std::format("            local results=( $(compgen -W '{}' -- \"$cur\") )\n", completions)
              << "            for r in \"${results[@]}\"; do\n"
              << "                COMPREPLY+=( \"${r// /\\\\ }\" )\n"
              << "            done\n"
              << "            return 0\n"
              << "            ;;\n";
        } else {
          cmds_ss << std::format(
              "        \"{}\") COMPREPLY=( $(compgen -W \"{}\" -- \"$cur\") ); return 0 ;;\n", path, completions
          );
        }
      }

      for (const auto& sub : cmd.subcommands) {
        std::string sub_path = path.empty() ? std::string(sub.name) : path + "_" + std::string(sub.name);
        generateBashCases(flags_ss, cmds_ss, sub, sub_path);
      }
    }
  } // namespace

  std::string generateFish(const cli_schema::CliCommand& root) {
    std::ostringstream ss;
    ss << "# Fish completion dynamically generated by Noctalia\n\n";

    ss
        << "function __fish_noctalia_using_command\n"
        << "    set -l cmd (commandline -opc)\n"
        << "    if test (count $cmd) -ne (count $argv)\n"
        << "        return 1\n"
        << "    end\n"
        << "    for i in (seq (count $argv))\n"
        << "        if test $cmd[$i] != $argv[$i]\n"
        << "            return 1\n"
        << "        end\n"
        << "    end\n"
        << "    return 0\n"
        << "end\n\n";

    ss << "complete -c noctalia -f\n\n";

    generateFishCommand(ss, root, "noctalia");

    return ss.str();
  }

  std::string generateZsh(const cli_schema::CliCommand& root) {
    std::ostringstream ss;
    ss << "#compdef noctalia\n\n";

    ss << "typeset -ga _NOCTALIA_TOP_COMMANDS=(\n";
    for (const auto& sub : root.subcommands) {
      ss << std::format("    '{}:{}'\n", sub.name, escapeQuotes(sub.summary));
    }
    ss << ")\n\n";

    std::function<void(const cli_schema::CliCommand&)> emitCommandArrays = [&](const cli_schema::CliCommand& cmd) {
      for (const auto& sub : cmd.subcommands) {
        if (!sub.subcommands.empty()) {
          ss << std::format("typeset -ga _NOCTALIA_{}_COMMANDS=(\n", toZshVarNamePart(sub.name));
          for (const auto& subSub : sub.subcommands) {
            ss << std::format("    '{}:{}'\n", subSub.name, escapeQuotes(subSub.summary));
          }
          ss << ")\n\n";
          emitCommandArrays(sub);
        }
      }
    };
    emitCommandArrays(root);

    ss
        << "_noctalia() {\n"
        << "    local context state state_descr line\n"
        << "    typeset -A opt_args\n\n"
        << "    _arguments -C \\\n"
        << "        '(-h --help)'{-h,--help}'[Show help message]' \\\n"
        << "        '1: :->cmds' \\\n"
        << "        '*:: :->args'\n\n"
        << "    case \"$state\" in\n"
        << "        cmds)\n"
        << "            _describe -t commands 'noctalia command' _NOCTALIA_TOP_COMMANDS\n"
        << "            ;;\n"
        << "        args)\n"
        << "            case $line[1] in\n";

    for (const auto& sub : root.subcommands) {
      ss << std::format("                {})\n", sub.name);
      if (!sub.subcommands.empty()) {
        ss << std::format("                    _noctalia_{}\n", sub.name);
      } else {
        ss << std::format("                    _arguments {}\n", buildArgumentsItems(sub));
      }
      ss << "                    ;;\n";
    }

    ss
        << "            esac\n"
        << "            ;;\n"
        << "    esac\n"
        << "}\n\n";

    for (const auto& sub : root.subcommands) {
      if (!sub.subcommands.empty()) {
        generateZshCommandsRecursive(ss, sub, "noctalia_" + std::string(sub.name));
      }
    }

    ss << "_noctalia \"$@\"\n";

    return ss.str();
  }

  std::string generateBash(const cli_schema::CliCommand& root) {
    std::ostringstream ss;
    std::ostringstream flags_ss;
    std::ostringstream cmds_ss;

    generateBashCases(flags_ss, cmds_ss, root, "");

    ss
        << "# Bash completion dynamically generated by Noctalia\n\n"
        << "_noctalia_completion()\n"
        << "{\n"
        << "    local cur prev words cword\n"
        << "    _init_completion -s 2>/dev/null || {\n"
        << "        cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
        << "        prev=\"${COMP_WORDS[COMP_CWORD-1]}\"\n"
        << "        words=(\"${COMP_WORDS[@]}\")\n"
        << "        cword=$COMP_CWORD\n"
        << "    }\n\n"
        << "    local i cmd=\"\"\n"
        << "    for ((i=1; i < cword; i++)); do\n"
        << "        local w=\"${words[i]}\"\n"
        << "        if [[ \"$w\" != -* ]]; then\n"
        << "            cmd=\"${cmd}${cmd:+_}$w\"\n"
        << "        fi\n"
        << "    done\n\n"
        << "    if [[ \"$cur\" == -* ]]; then\n"
        << "        case \"$cmd\" in\n"
        << flags_ss.str()
        << "        esac\n"
        << "        return 0\n"
        << "    fi\n\n"
        << "    case \"$cmd\" in\n"
        << cmds_ss.str()
        << "    esac\n\n"
        << "    COMPREPLY=( $(compgen -f -- \"$cur\") )\n"
        << "    return 0\n"
        << "}\n\n"
        << "complete -F _noctalia_completion noctalia\n";

    return ss.str();
  }
} // namespace noctalia::completions
