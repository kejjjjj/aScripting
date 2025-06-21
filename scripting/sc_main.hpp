#pragma once

#include <string>
#include <memory>
#include <vector>

[[nodiscard]] bool SC_Execute(const std::string& script);
[[nodiscard]] bool SC_ExecuteFile(const std::string& script);

