//
// Created by 46769 on 2021-01-20.
//

#include "gap_buffer.hpp"
#include "text.hpp"
#include <ranges>

#include <algorithm>
#include <cassert>
#include <functional>

GapBuffer::GapBuffer(int starting_capacity, int gap_size) : state{.gap = {0, gap_size}, .cursor = {}, .size = 0, .cap = starting_capacity, .gap_starting_size = gap_size}, data(nullptr) {
    data = new char[starting_capacity];
    state.cursor.gap_pos = &state.gap.begin;
}

int GapBuffer::pos() const {
    GAP_BUFFER_ASSERT(*state.cursor.gap_pos == state.gap.begin);
    return state.gap.begin;
}

int GapBuffer::line() const {
    return state.cursor.line;
}

[[maybe_unused]] int GapBuffer::col_pos() const {
    return state.cursor.col;
}

char GapBuffer::get_ch() const {
    if(state.cursor.pos == size()) return 0;
    return get_at(state.cursor.pos);
    // return data[state.gap.begin + state.gap.length];
}

int GapBuffer::gap_length() const {
    return state.gap.length;
}


void GapBuffer::insert_str(std::string_view v) {
    gap_commit();
    const auto insertSize = v.size();
    auto cap = capacity();
    if (insertSize + size() >= cap) {
        resize_buffer_capacity((insertSize + cap) * 2);
    }
    if (insertSize < gap_length()) {
        std::memcpy(data + state.gap.begin, v.data(), insertSize);
        state.gap.begin += insertSize;
        state.gap.length -= insertSize;
        state.size += insertSize;
        state.cursor.pos += insertSize;
    } else {
        for (auto c : v) insert(c);
    }
}

void GapBuffer::insert(char ch) {
    gap_commit();
    if (size() == capacity()) {
        resize_buffer_capacity(capacity() * 2);
    }
    if (state.gap.length == 0) {
        resize_gap(state.gap_starting_size);
    }
    data[state.gap.begin] = ch;
    if (ch == '\n') {
        state.cursor.line++;
        state.cursor.col = 0;
    }
    state.cursor.pos++;
    state.gap.begin++;
    state.gap.length--;
    state.size++;
}

void GapBuffer::resize_gap(int gap_size) {
    if (size() + gap_size >= capacity()) {
        resize_buffer_capacity((capacity() + gap_size) * 2);
    }
    GAP_BUFFER_ASSERT(size() + gap_size < capacity());

    auto elements_to_shift = size() - (state.gap.begin + state.gap.length);
    auto begin = data + (state.gap.begin + state.gap.length);
    auto gap_end = begin + gap_size;
    /// dst / src are overlapping, memmove must be used, memcpy is UB
    std::memmove(gap_end, begin, elements_to_shift);
    state.gap.length = gap_size;
}

int GapBuffer::size() const {
    return state.size;
}


void GapBuffer::move_gap_cursor_to(int index) {
    if (index == state.gap.begin) return;
    GAP_BUFFER_ASSERT(index != state.gap.begin && index <= size() && index >= 0);// this assert is to see that we don't do dumb "move to where we are"
    index = std::max(0, index);
    if (index > state.gap.begin + state.gap.length) {
        auto items_to_move = index - (state.gap.begin + gap_length());
        auto begin = data + state.gap.begin;
        auto e = begin + state.gap.length;
        std::memmove(begin, e, items_to_move);
        state.gap.begin = index;
    } else if (index > state.gap.begin && index <= (state.gap.begin + state.gap.length)) {
        auto items_to_move = index - state.gap.begin;
        auto begin = data + state.gap.begin;
        auto e = begin + state.gap.length;
        std::memmove(begin, e, items_to_move);
        state.gap.begin = index;
    } else {
        auto items_to_move = state.gap.begin - index;
        auto begin_move = data + index;
        auto move_to = begin_move + state.gap.length;
        std::memmove(move_to, begin_move, items_to_move);
        state.gap.begin = index;
    }
}

int GapBuffer::capacity() const {
    return state.cap;
}


void GapBuffer::resize_buffer_capacity(int new_size) {
    GAP_BUFFER_ASSERT(new_size > size());
    auto heap = new char[new_size];
    if (size() == 0) {
        delete data;
        data = heap;
    } else {
        // For now, our intrinsics version does the exact same thing for correctness measures
        /// TODO(sse/avx): copy larger sections using SIMD instructions. Can speed up by a lot
#ifdef INTRINSICS
        std::memcpy(heap, data, capacity());
#else
        std::memcpy(heap, data, capacity());
#endif
        auto foo = data;
        data = heap;
        delete foo;
        state.cap = new_size;
    }
}

void GapBuffer::erase_forward(int char_count) {
    gap_commit();
    if (state.gap.begin < size()) {
        // TODO: update meta data for remaining lines in buffer, once meta data exists for this buffer type
        state.gap.length++;
        state.size--;
    }
}
int GapBuffer::remaining_space() const {
    return capacity() - size();
}

void GapBuffer::erase_backward(int char_count) {
    gap_commit();
    if ((state.gap.begin - char_count) > 0) {
        state.cursor.pos -= char_count;
        state.gap.begin -= char_count;
        state.gap.length += char_count;
        state.size -= char_count;
    } else {
        auto diff = state.gap.begin;
        state.cursor.pos = 0;
        state.gap.begin = 0;
        state.gap.length += diff;
        state.size -= diff;
    }
}

void GapBuffer::move_gap_cursor_back(int steps) {
    move_gap_cursor_to(state.gap.begin - steps);
}
void GapBuffer::move_gap_cursor_forward(int steps) {
    move_gap_cursor_to(state.gap.begin + steps);
}
void GapBuffer::clear() {
    state.reset();
}

std::optional<int> GapBuffer::find(std::string_view search) {
    auto begin = data;
    auto e = data + gap_begin();
    auto it = std::search(begin, e, std::boyer_moore_searcher(search.begin(), search.end()));
    if (it != e) {
        return std::distance(begin, it);
    }
    // c++ implicit conversions suck. The compiler *should* be able to figure out, that the first param (gap_begin()) is int
    // the second param search.size() is size_t, so how does it go on? Well, the compiler also sees that I am asking for >=0,
    // which is *literally* always true for size_t, yet no warning, no message, nothing. But that's not even the point,
    // I am *explicitly* asking for >=0 in hardcoded literal. It is always true if it decides to cast the expression to size_t, it doesn't make sense.
    if (gap_begin() - (int) search.size() >= 0) {
        auto searchSize = search.size();
        // Scan across gap boundary naively/brute force
        for (auto i = gap_begin() - searchSize; i < gap_begin() + searchSize; i++) {
            auto matches = false;
            for (auto inner = 0; inner < searchSize; inner++) {
                auto ch = get_at(i + inner);
                char needle_ch = search[inner];
                if (ch != needle_ch) {
                    matches = false;
                    break;
                } else {
                    matches = true;
                }
            }
            if (matches) {
                return i;
            }
        }
        auto seg_begin = data + gap_begin() + gap_length();
        auto seg_end = data + size() + gap_length();
        auto iter = std::search(seg_begin, seg_end, std::boyer_moore_searcher(search.begin(), search.end()));
        if (iter != seg_end) {
            auto diff = std::distance(seg_begin, iter);
            return diff + gap_begin();
        }
    } else {// the cursor is in a position, where the needle size can not fit before it
        auto searchSize = search.size();
        for (auto i = 0; i < searchSize + gap_begin(); i++) {
            auto matches = false;
            for (auto inner = 0; inner < searchSize; inner++) {
                auto ch = get_at(i + inner);
                char needle_ch = search[inner];
                if (ch != needle_ch) {
                    matches = false;
                    break;
                } else {
                    matches = true;
                }
            }
            if (matches) {
                return i;
            }
        }
        auto seg_begin = data + gap_begin() + gap_length();
        auto seg_end = data + size() + gap_length();
        auto iter = std::search(seg_begin, seg_end, std::boyer_moore_searcher(search.begin(), search.end()));
        if (iter != seg_end) {
            auto diff = std::distance(seg_begin, iter);
            return diff + gap_begin();
        }
    }
    return {};
}

/// TODO: look for ways to optimize this. Due to the complexity of the gap in the gap buffer, right now we just do brute force when using find_from
std::optional<int> GapBuffer::find_from(std::string_view search, std::optional<int> optionalPos) const {
    auto sz = size();
    int searchSize = search.size();
    auto begin = optionalPos.value_or(0);
    bool matches = false;
    for(; begin < sz; begin++) {
        for (auto inner = 0; inner < searchSize; inner++) {
            auto ch = get_at(begin + inner);
            char needle_ch = search[inner];
            if (ch != needle_ch) {
                matches = false;
                break;
            } else {
                matches = true;
            }
        }
        if(matches) return begin;
    }
    return {};
}

std::optional<int> GapBuffer::find_ch_from(char item, std::optional<int> pos) const {
    auto sz = size();
    auto begin = pos.value_or(0);
    for(; begin < sz; begin++) {
        if(item == get_at(begin)) return begin;
    }
    return {};
}


char &GapBuffer::operator[](int characterIndex) {
    GAP_BUFFER_ASSERT(characterIndex < size());
    if (characterIndex >= state.gap.begin) {
        auto result = data[characterIndex + gap_length()];
        return data[characterIndex + gap_length()];
    } else {
        return data[characterIndex];
    }
}

char GapBuffer::get_at(int pos) const {
    if (pos < state.gap.begin) return data[pos];
    return data[pos + gap_length()];
}

char& GapBuffer::get_at_ref(int pos) {
    if (pos < state.gap.begin) return data[pos];
    return data[pos + gap_length()];
}

int GapBuffer::gap_begin() const {
    return state.gap.begin;
}

std::string GapBuffer::clone_range(int begin, int length) const {
    if(begin + length < gap_begin() || begin > (gap_begin() + gap_length())) {
        std::string res;
        res.reserve(length);
        std::copy(data+begin, data+begin+length, std::back_inserter(res));
        return res;
    } else {
        std::string res;
        for(auto p = begin; p < begin + length; p++) {
            res.push_back(get_at(p));
        }
        return res;
    }
}

void GapBuffer::gap_commit() {
    move_gap_cursor_to(state.cursor.pos);
}

void GapBuffer::move_cursor_to(int index) {
    GAP_BUFFER_ASSERT(index <= size());
    state.cursor.pos = index;
}

void GapBuffer::move_cursor_forward(int steps) {
    state.cursor.pos = std::min(state.cursor.pos + steps, size());
}

void GapBuffer::move_cursor_backward(int steps) {
    state.cursor.pos = std::max(state.cursor.pos - steps, 0);
}
