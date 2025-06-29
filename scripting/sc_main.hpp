#pragma once

#include <string>
#include <memory>
#include <vector>

[[nodiscard]] bool SC_Execute(const std::string& script);
[[nodiscard]] bool SC_ExecuteFile(const std::string& script);

namespace Varjus { struct State; }

[[nodiscard]] bool SC_ExecuteAsynchronously(Varjus::State& state, const std::string& script);
[[nodiscard]] bool SC_ExecuteFileAsynchronously(Varjus::State& state, const std::string& script);