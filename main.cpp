#include <cassert>
#define FMT_ENFORCE_COMPILE_STRING
#include <array>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/format.h>
#include <gb/gap_buffer.hpp>
#include <string>
#include <string_view>
#include <unittest/unit_test.hpp>
#include <vector>

using namespace fmt::literals;
using namespace std::string_view_literals;

void vlogln(const char *file, int line, fmt::string_view format,
            fmt::format_args args) {
    fmt::print(FMT_STRING("{}: {}: "), file, line);
    fmt::vprint(format, args);
    fmt::print(FMT_STRING("\n"));
}

template<typename S, typename... Args>
void log(const char *file, int line, const S &format, Args &&...args) {
    vlogln(file, line, format,
           fmt::make_args_checked<Args...>(format, args...));
}

#define PRINTLN(format, ...) log(__FILE__, __LINE__, FMT_STRING(format), __VA_ARGS__)

#define FORMAT(str, ...) fmt::format(FMT_STRING(str), __VA_ARGS__)

constexpr auto prefix = "void __cdecl "sv;
using namespace fmt::literals;
#define STRIP_FUNCSIG(funcsig)
#define FNSIG std::string_view{__FUNCSIG__};

// For inserting some longer text than just "Hello world!"
constexpr std::string_view movement_header() {
    return
R"(enum CursorDirection { Forward, Back };
enum TextRep { Char, Word, Line, Block, File };
enum Boundary { Inside, Outside };

struct Movement {
    Movement(size_t count, TextRep elem_type, CursorDirection dir) : count(count), construct(elem_type), dir(dir) {}
    size_t count;
    TextRep construct;
    CursorDirection dir;
    Boundary boundary;
    static Movement Char(size_t count, CursorDirection dir);
    static Movement Word(size_t count, CursorDirection dir);
    static Movement Line(size_t count, CursorDirection dir);
    /* TODO: implement these when it's reasonable to do so
    static Movement Block(size_t count, CursorDirection dir) { return Movement{count, TextRep::Block, dir}; }
    static Movement File(CursorDirection dir) { return Movement{0, TextRep::File, dir}; }
     */
};)";
}

std::vector<GapBuffer> setup_gapbuffers() {
    std::vector<GapBuffer> gapBuffers{};
    for (auto gap_size = 2; gap_size <= 32; gap_size *= 2) {
        for (auto starting_cap = gap_size; starting_cap <= 64; starting_cap *= 2) {
            gapBuffers.emplace_back(starting_cap, gap_size);
        }
    }
    return gapBuffers;
}


void larger_insertion_test() {
    BeginUnitTest();
    auto gbs = setup_gapbuffers();
    constexpr auto assertEqualsFn = [](auto pos, auto gb_ch, auto str_view_ch) {
        if (gb_ch != str_view_ch) {
            PRINTLN("assertion failed at buffer text-position {} - expected: {} | found: {}", pos, str_view_ch, gb_ch);
            return false;
        }
        return true;
    };
    constexpr auto movement_header_data = movement_header();
    auto SubUnitTest = 0;
    try {
        for (auto &gb : gbs) {
            for (auto ch : movement_header_data) {
                gb.insert(ch);
            }

            auto gb_contents = gb.collect_from(0, gb.size(), [](auto &vec, auto c) {
                vec.push_back(c);
            });
            gb_contents.push_back('\0');// c-string terminator, so that we can call .data() on vector and pretend it's a string
            UnitTestPush(fmt::format(FMT_STRING("Contents do not match: {} != {}"), movement_header_data, gb_contents.data()), gb.debug_assert(movement_header_data, assertEqualsFn));
            SubUnitTest++;
        }
    } catch (std::exception &e) {
        fmt::print(FMT_STRING("Caught assertion error for sub unit test {}: {}\n"), SubUnitTest, e.what());
    }
}

void gb_motions_test() {
    BeginUnitTest();
    constexpr auto assertEqualsFn = [](auto pos, auto gb_ch, auto str_view_ch) {
      if (gb_ch != str_view_ch) {
          PRINTLN("assertion failed at buffer text-position {} - expected: {} | found: {}", pos, str_view_ch, gb_ch);
          return false;
      }
      return true;
    };
    std::string_view buffer_contents = "Hello world";
    constexpr auto after_edits = "Hello, world!";
    GapBuffer gb{10, 6};
    gb.insert_str(buffer_contents);

    auto cnt = 0;
    gb.move_cursor_to(0);

    auto ch = gb.get_ch();
    UnitTestPush(FORMAT("Expected 'H' found {}", ch), ch == 'H');

    gb.move_cursor_to(4);
    ch = gb.get_ch();
    UnitTestPush(FORMAT("Expected 'o' found {}", ch), ch == 'o');

    gb.move_cursor_to(3);
    gb.move_cursor_to(4);
    ch = gb.get_ch();
    UnitTestPush(FORMAT("Expected 'o' found {}", ch), ch == 'o');

    gb.move_cursor_to(6);
    ch = gb.get_ch();
    UnitTestPush(FORMAT("Expected 'w' found {}", ch), ch == 'w');
    gb.move_cursor_to(5);
    gb.insert(',');

    gb.move_cursor_to(7);
    ch = gb.get_ch();
    UnitTestPush(FORMAT("Expected 'w' found {}", ch), ch == 'w');

    gb.move_cursor_to(gb.size());
    ch = gb.get_ch();
    UnitTestPush(FORMAT("Expected C-string null terminator found {}", ch), ch == 0);

    gb.insert('!');

    auto gb_contents = gb.collect_from(0, gb.size(), [](auto &vec, auto c) {
      vec.push_back(c);
    });
    gb_contents.push_back('\0');// c-string terminator, so that we can call .data() on vector and pretend it's a string
    UnitTestPush(fmt::format(FMT_STRING("Contents do not match: {} != {}"), after_edits, gb_contents.data()), gb.debug_assert(after_edits, assertEqualsFn));

}

void insert_str_smaller_than_gap_test() {
    BeginUnitTest();
    constexpr auto world = "world!"sv;
    constexpr auto assertEqualsFn = [](auto pos, auto gb_ch, auto str_view_ch) {
        if (gb_ch != str_view_ch) {
            PRINTLN("assertion failed at buffer text-position {} - expected: {} | found: {}", pos, str_view_ch, gb_ch);
            return false;
        }
        return true;
    };

    auto gapBuffers = setup_gapbuffers();
    auto SubUnitTest = 0;
    try {
        for (auto &gb : gapBuffers) {

            for (auto c : world) gb.insert(c);
            gb.move_cursor_to(0);
            gb.insert_str("Hello ");
            auto assert_result = "Hello world!"sv;

            auto gb_contents = gb.collect_from(0, gb.size(), [](auto &vec, auto c) {
                vec.push_back(c);
            });
            gb_contents.push_back('\0');// c-string terminator, so that we can call .data() on vector and pretend it's a string
            UnitTestPush(fmt::format(FMT_STRING("Contents do not match: {} != {}"), assert_result, gb_contents.data()), gb.debug_assert(assert_result, assertEqualsFn));
            SubUnitTest++;
        }
    } catch (std::exception &e) {
        fmt::print(FMT_STRING("Caught assertion error for sub unit test {}: {}\n"), SubUnitTest, e.what());
    }
}
void remove_forward_backward_test() {
    BeginUnitTest();
    constexpr auto assertEqualsFn = [](auto pos, auto gb_ch, auto str_view_ch) {
        if (gb_ch != str_view_ch) {
            PRINTLN("assertion failed at buffer text-position {} - expected: {} | found: {}", pos, str_view_ch, gb_ch);
            return false;
        }
        return true;
    };

    constexpr auto hello = "HellO"sv;
    constexpr auto world = " world!"sv;
    constexpr auto assert_result = "Hello world!"sv;
    constexpr auto assert_result2 = "HeLLo world!"sv;

    auto gapBuffers = setup_gapbuffers();
    for (auto &gb : gapBuffers) {
        for (auto c : hello) gb.insert(c);
        // gb.move_cursor_to(4);
        gb.move_cursor_to(4);

        gb.erase_forward();
        gb.insert('o');
        gb.insert_str(world);
        auto gb_contents = gb.collect_from(0, gb.size(), [](auto &vec, auto c) {
            vec.push_back(c);
        });
        gb_contents.push_back('\0');// c-string terminator, so that we can call .data() on vector and pretend it's a string
        UnitTestPush(fmt::format(FMT_STRING("Contents do not match: {} != {}"), assert_result, gb_contents.data()), gb.debug_assert(assert_result, assertEqualsFn));

        gb.move_cursor_to(4);
        gb.erase_backward(2);
        gb.insert_str("LL");


        gb_contents = gb.collect_from(0, gb.size(), [](auto &vec, auto c) {
            vec.push_back(c);
        });
        gb_contents.push_back('\0');// c-string terminator, so that we can call .data() on vector and pretend it's a string
        UnitTestPush(fmt::format(FMT_STRING("Contents do not match: {} != {}"), assert_result2, gb_contents.data()), gb.debug_assert(assert_result2, assertEqualsFn));
    }
}
void operator_brackets_test() {
    BeginUnitTest();
    constexpr auto contents = "hello world!";
    constexpr auto result = "HELLO WORLD!";
    constexpr auto assertEqualsFn = [](auto pos, auto gb_ch, auto str_view_ch) {
        if (gb_ch != str_view_ch) {
            PRINTLN("assertion failed at buffer text-position {} - expected: {} | found: {}", pos, str_view_ch, gb_ch);
            return false;
        }
        return true;
    };

    auto gbs = setup_gapbuffers();
    for (auto &gb : gbs) {
        gb.insert_str(contents);
    }

    for (auto &gb : gbs) {
        gb.for_each_character([](auto &c) {
            c = std::toupper(c);
        });
        auto gb_contents = gb.collect_from(0, gb.size(), [](auto &vec, auto c) {
            vec.push_back(c);
        });
        gb_contents.push_back('\0');
        UnitTestPush(fmt::format(FMT_STRING("Contents do not match: {} != {}"), result, gb_contents.data()), gb.debug_assert(result, assertEqualsFn));
        // gb.debug_print_contents([](auto ch) { std::cout << ch; }, true);
    }
}

// !----- helper functions for unit test test_search and test_search_from
bool searcher(GapBuffer &gb, int posToMoveTo, std::string_view needle, bool printFound = false) {
    gb.move_cursor_to(posToMoveTo);
    auto pos = gb.find(needle);
    if (pos) {
        if (printFound) {
            PRINTLN("\nGap buffer cap: {} Gap {{{} / {}}} Gap size setting: {} cursor pos: {} \t found item at {}\n", gb.capacity(), gb.gap_begin(), gb.gap_length(), gb.gap_size_setting(), posToMoveTo, pos.value());
        }
        auto begin = pos.value();
        auto word = gb.collect_x_from(begin, needle.size(), [](auto &vec, auto ch) {
            vec.push_back(ch);
        });
        word.push_back('\0');
        if (word.data() != needle) {
            gb.debug_print_contents([](auto ch) { fmt::print(FMT_STRING("{}"), ch); }, true);
            fflush(stdout);
            throw std::runtime_error{"Test failed"};
        }
        for (auto i = begin; i < needle.size(); i++) {
            if (gb[i] != needle[i]) {
                throw std::runtime_error{fmt::format(FMT_STRING("Collected contents did not match: gb[{0}] != needle[{0}]: {1} != {2}"), i, gb[i], needle[i])};
            }
        }
        return true;
    } else {
        fmt::print(FMT_STRING("Did not find item {} when in position {}\n"), needle, posToMoveTo);
        return false;
    }
}
bool searcher_from(GapBuffer &gb, int posToMoveTo, int searchFrom, std::string_view needle, bool printFound = false) {
    gb.move_cursor_to(posToMoveTo);
    auto validatedPos = gb.find(needle);
    if (validatedPos) {
        auto pos = gb.find_from(needle, searchFrom);
        if (pos) {
            if (searchFrom <= validatedPos.value()) {
                if (printFound) {
                    PRINTLN("\nGap buffer cap: {} Gap {{{}/{}}} Gap size setting: {} cursor pos: {} \t found item at {}\n", gb.capacity(), gb.gap_begin(), gb.gap_length(), gb.gap_size_setting(), posToMoveTo, pos.value());
                }

                auto begin = pos.value();
                auto word = gb.collect_x_from(begin, needle.size(), [](auto &vec, auto ch) {
                    vec.push_back(ch);
                });
                word.push_back('\0');
                if (word.data() != needle) {
                    gb.debug_print_contents([](auto ch) { fmt::print(FMT_STRING("{}"), ch); }, true);
                    fflush(stdout);
                    throw std::runtime_error{"Test failed"};
                }
                for (auto i = begin; i < needle.size(); i++) {
                    if (gb[i] != needle[i]) {
                        throw std::runtime_error{fmt::format(FMT_STRING("Collected contents did not match: gb[{0}] != needle[{0}]: {1} != {2}"), i, gb[i], needle[i])};
                    }
                }
                return true;
            } else {// this means we are searching from _after_ where the needle exists, thus this test should return "true" here, because we should not find it
                if (pos) return false;
                return true;
            }
        } else {
            if (searchFrom <= validatedPos.value()) {
                return false;
            } else {
                return true;
            }
        }
    } else {
        fmt::print(FMT_STRING("Did not find item {} when in position {}\n"), needle, posToMoveTo);
        fflush(stdout);
        return false;
    }
}
// -----!

/// Inserts the string "hello world says c++" into gapbuffers, with varying starting capacitys and varying gap sizes
/// Then iterates over all gap buffers, and for each gap buffer, moves the cursor from the beginning -> end, 1 step at the time,
/// and searching for the needle "world". This is due to the nature of the gap buffer, since the cursor actually shifts elements around
/// in the gapbuffer, we must verify that searching for a word, which is split on the boundary of the gap, is actually found.
void search_test() {
    BeginUnitTest();
    constexpr auto content = "hello world says c++"sv;
    std::vector<GapBuffer> gapBuffers = setup_gapbuffers();
    for (auto &g : gapBuffers) g.insert_str(content);

    // hello is the first word in the test, thus asserts if searching for an item works when gap_begin() - needle.size() < 0
    std::array needles{"hello", "world", "ll", "l"};

    try {
        auto SubUnitTest = 0;
        for (const auto &needle : needles) {
            auto gbIndex = 0;
            for (auto &gb : gapBuffers) {
                auto cSize = gb.size();
                for (auto i = 0; i < cSize; i++) {

                    auto failMsg = fmt::format(FMT_STRING("failed to find needle, when gb cursor was in position: {} for gap buffer {} with capacity: {} and gap size: {{{}/{}}}n"),
                                               i, gbIndex, gb.capacity(), gb.gap_length(), gb.gap_size_setting());
                    UnitTestPush(failMsg, searcher(gb, i, needle));
                    SubUnitTest++;
                }
                gbIndex++;
            }
        }
    } catch (std::exception &e) {
        fmt::print(FMT_STRING("Assertion failed: {}\n"), e.what());
        fflush(stdout);
    }
}

void search_from_test() {
    BeginUnitTest();
    constexpr auto content = "hello world says c++"sv;
    std::vector<GapBuffer> gapBuffers = setup_gapbuffers();
    for (auto &gb : gapBuffers) gb.insert_str(content);

    // hello is the first word in the test, thus asserts if searching for an item works when gap_begin() - needle.size() < 0
    std::array needles{"hello", "world", "ll"};

    try {
        auto subUnitTests = 0;
        for (const auto &needle : needles) {
            auto gbIndex = 0;
            for (auto &gb : gapBuffers) {
                auto cSize = gb.size();
                for (auto i = 0; i < cSize; i++) {
                    for (auto searchFrom = 0; searchFrom < cSize; searchFrom++) {

                        auto failMsg = fmt::format(FMT_STRING("failed to find '{}', Cursor position: {} - GB {} - Capacity: {} - Gap Size: {{{}/{}}} - Search start from {}\n"), needle, i, gbIndex, gb.capacity(), gb.gap_length(), gb.gap_size_setting(), searchFrom);

                        UnitTestPush(failMsg, searcher_from(gb, i, searchFrom, needle));
                        subUnitTests++;
                    }
                }
                gbIndex++;
            }
        }
    } catch (std::exception &e) {
        fmt::print(FMT_STRING("Assertion failed: {}\n"), e.what());
        fflush(stdout);
    }
}

void non_mutating_cursor_ops_test() {
    BeginUnitTest();
    GapBuffer gb{32};
    gb.insert_str("hello world");
    gb.move_cursor_to(0);
    auto stable = gb.state.gap.begin;
    for (auto i = 0; i < gb.size(); i++) {
        if (i == stable) {
            continue;
        } else {
            gb.move_cursor_to(i);
            UnitTestPush(FORMAT("Non mutating cursor pos, is not correct, gap & cursor should not be equal {} != {}", gb.state.gap.begin, gb.state.cursor.pos), gb.state.gap.begin != gb.state.cursor.pos);
        }
    }

    // gb.move_cursor_to(0);
    for (auto i = 0; i < gb.size(); i++) {
        UnitTestPush(FORMAT("Non mutating cursor pos, is not correct {} != {}", gb.state.gap.begin, *gb.state.cursor.gap_pos), gb.state.gap.begin == *gb.state.cursor.gap_pos);
    }
}

void clear_test() {
    BeginUnitTest();
    constexpr auto content_test = "hello world"sv;
    GapBuffer gb{32};
    gb.insert_str(content_test);
    UnitTestPush(FORMAT("Size expected: {}, got: {}", content_test.size(), gb.size()), content_test.size() == gb.size());
    auto cap = gb.capacity();
    auto sz = gb.size();
    gb.clear();
    UnitTestPush(FORMAT("Capacity expected: {}, got: {}", cap, gb.capacity()), cap == gb.capacity());
    gb.insert_str(content_test);
    UnitTestPush(FORMAT("Size expected: {}, got: {}", content_test.size(), gb.size()), content_test.size() == gb.size());
    gb.clear();
    UnitTestPush(FORMAT("Size expected: {}, got: {}", 0, gb.size()), 0 == gb.size());
}

int main() {
    try {
        remove_forward_backward_test();
        operator_brackets_test();
        search_test();
        search_from_test();
        larger_insertion_test();
        insert_str_smaller_than_gap_test();
        non_mutating_cursor_ops_test();
        clear_test();
        gb_motions_test();
    } catch(std::exception& e) {
        fmt::print(FMT_STRING("Error caught: {}\n"), e.what());
        fflush(stdout);
    }
    return 0;
}
