//
// Created by 46769 on 2021-01-20.
//

#pragma once
#include <optional>
#include <string_view>
#include <vector>

#define GAP_BUFFER_ASSERT(BooleanExpr) assert(BooleanExpr)

struct BufferCursor {
    const int * gap_pos{nullptr}; // points to GapBuffer::state.gap.begin
    int pos{0};
    int col{0};
    int line{0};
};

struct Gap {
    int begin;
    int length;
};


/// Template for a function that takes a std::vector<char>&, char => fn(std::vector<char>& out, char ch_input);
template<typename Fn> concept Collector = requires(Fn fn) {
    fn(std::declval<std::vector<char>&>(), char{});
};

// TODO: add clone_range(begin, end)

class GapBuffer {
public:
    explicit GapBuffer(int starting_capacity, int gap_size = 16);

    /// Data member functions, either operates or retrieves the data
    char get_ch() const;
    /// Inserts character at cursor position
    void insert(char ch);
    /// Inserts a range of characters, in std::string_view
    void insert_str(std::string_view);

    /// erases char(s), forward as if user pressed "DELETE", if BACKSPACE-action is wanted, use erase_backward()
    void erase_forward(int char_count = 1);
    /// erase char(s), backward as if user pressed "BACKSPACE", if DELETE-action is wanted, use erase_forward()
    void erase_backward(int char_count = 1);
    /// clears the buffer, this is in a sense a "no-op", since it only sets the gap cursor to {0, buffer capacity}
    void clear();
    /// Clones the data between text positions [begin, begin+length)
    std::string clone_range(int begin, int length) const;

    /* State member functions, either operates, or retrieves information about the state object */

    /// Returns position of cursor
    int pos() const;
    /// Returns the line number of where the cursor is
    int line() const;
    /// Returns the column position on current line
    int col_pos() const;
    /// Returns data contents size
    int size() const;
    /// Returns buffer size
    int capacity() const;

    /// "Commits" the gap cursor to the text-cursor position (i.e. moves the gap to text-cursor position). One must remember that the
    /// text cursor is an abstract cursor, it is not an absolute, so any position P that the cursor points to, is not necessarily index I in the data buffer data[]
    void move_cursor_to(int index);
    void move_cursor_forward(int steps);
    void move_cursor_backward(int steps);

    /// Find first instance of search in the buffer
    std::optional<int> find(std::string_view search);

    /// Find first instance of search in the buffer, starting from (optional) pos
    std::optional<int> find_from(std::string_view search, std::optional<int> pos = {}) const;
    std::optional<int> find_ch_from(char item, std::optional<int> pos = {}) const;

    /// Returns character at characterIndex - meaning, this does not give access to the gap, inside of the gap buffer, only it's actual string contents
    char& operator[](int characterIndex);

    char get_at(int pos) const;
    char& get_at_ref(int pos);

    inline int gap_begin() const;
    inline int gap_length() const;

    // TODO: remove this. ONLY meant for debugging purposes
    inline int gap_size_setting() const {
        return state.gap_starting_size;
    }

    // State, meant to be private once the debugging process is done
    struct {
        Gap gap;
        BufferCursor cursor;
        int size;
        int cap;
        const int gap_starting_size;
        inline void reset() {
            gap.begin = 0;
            gap.length = cap;
            size = 0;
            cursor.pos = 0;
            cursor.col = 0;
            cursor.line = 0;
        }
    } state;
private:
    char *data;
    void resize_gap(int gap_size);
    void resize_buffer_capacity(int new_size);

    /// Commits the gap cursor to the text-cursor position. The separation of the gap vs text cursor is so that the user of the interface
    /// doesn't have to be concerned with where the gap is (currently), and so that when displayed for instance, the cursor can freely be moved around,
    /// without the shifting of elements around, until an actual edit is made
    void gap_commit();

    /* Buffer motions. Internal bookkeeping of the data. The user doesn't want to be bogged down with this, but instead just use the interface
     * as if it was one continous stream of characters, thus, the user's idea of a cursor must work like that as well. So the gap cursor is just for us to know and deal with. */
    /// Moves cursor to position
    void move_gap_cursor_to(int index);
    /// Moves cursor steps forward. If steps lies outside the range of the buffer, it clamps down to land within the range
    void move_gap_cursor_back(int steps);
    /// Moves cursor steps backward. If steps lies outside the range of the buffer, it clamps down to land within the range
    void move_gap_cursor_forward(int steps);

    int remaining_space() const;
    // This is put down here to not clutter up the interface
public:

    template <Collector Fn>
    std::vector<char> collect_from(int pos, int to, Fn fn) {
        std::vector<char> result{};
        auto& Self = *this;
        for(auto i = pos; i < to; i++) {
            fn(result, char{Self[i]});
        }
        return result;
    }

    template <Collector Fn>
    std::vector<char> collect_x_from(int pos, int x, Fn fn) {
        std::vector<char> result{};
        auto& Self = *this;
        auto to = pos + x;
        for(auto i = pos; i < to; i++) {
            auto ch = Self[i];
            fn(result, ch);
        }
        return result;
    }

    template <typename Fn>
    void for_each_character(Fn fn) {
        const auto sz = size();
        for(auto i = 0; i < sz; i++) {
            fn(this->operator[](i));
        }
    }


    /// Applies fn to each character in this buffer, including the gap, which in this case is represnted by characters of '_' (underscores)
    template<typename PrintFn>
    void debug_print_contents(PrintFn fn, bool print_gap_characters = true) {
        for (auto i = 0; i < state.gap.begin; i++) {
            fn(data[i]);
        }

        if (print_gap_characters) {
            // Prints the gap cursor
            for (auto i = state.gap.begin; i < state.gap.begin + state.gap.length; i++) {
                fn('_');
            }
        }
        for (auto i = state.gap.begin + state.gap.length; i < size() + state.gap.length; i++) {
            fn(data[i]);
        }
    }

#ifdef GBDEBUG
    template<typename DebugFn>
    bool debug_assert(std::string_view contents_match, DebugFn fn) {
        if (size() != contents_match.size()) {
            return false;
        }
        auto begin = contents_match.begin();
        for (auto i = 0; i < state.gap.begin; i++, begin++) {
            if (fn(i, data[i], *begin) == false) {
                return false;
            }
        }
        for (auto i = state.gap.begin + state.gap.length; i < size() + state.gap.length; i++, begin++) {
            if (fn(i, data[i], *begin) == false) {
                return false;
            }
        }
        return true;
    }
#endif
};

// 0123456789ABCDEF
// hello ___world
// gap.begin = 6
// gap.len = 3
// at_cursor() = data[gap.begin + gap.len = 9] => 'w'