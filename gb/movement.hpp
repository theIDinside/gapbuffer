//
// Created by 46769 on 2021-01-20.
//


#pragma once

enum CursorDirection { Forward, Back };
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
};