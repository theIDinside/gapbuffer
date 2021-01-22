//
// Created by 46769 on 2021-01-20.
//

#include "movement.hpp"
Movement Movement::Char(size_t count, CursorDirection dir) {
    return Movement{count, TextRep::Char, dir};
}
Movement Movement::Word(size_t count, CursorDirection dir) {
    return Movement{count, TextRep::Word, dir};
}
Movement Movement::Line(size_t count, CursorDirection dir) {
    return Movement{count, TextRep::Line, dir};
}
