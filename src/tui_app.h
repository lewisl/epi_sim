#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ftxui/dom/elements.hpp"

std::optional<int> choose_menu(std::string_view prompt,
                               const std::vector<std::string>& entries,
                               std::string_view case_label = {});

std::optional<std::string> ask_text(std::string_view prompt);

void clear_terminal_lines(int line_count, bool leave_blank_line = true);
void clear_menu_widget(std::size_t entry_count);

void print_element(ftxui::Element element);
void print_command_card(std::string_view command_text);
void print_output_boundary(bool leading_blank);
