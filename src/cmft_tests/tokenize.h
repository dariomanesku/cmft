/*
 * Copyright 2014-2015 Dario Manesku. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef CMFT_TOKENIZE_H_HEADER_GUARD
#define CMFT_TOKENIZE_H_HEADER_GUARD

/// Code from: https://github.com/bkaradzic/bgfx/blob/master/tools/geometryc/tokenizecmd.cpp
/*
 * Copyright 2012-2014 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

const char* tokenizeCommandLine(const char* _commandLine, char* _buffer, uint32_t& _bufferSize, int& _argc, char* _argv[], int _maxArgvs, char _term)
{
    int argc = 0;
    const char* curr = _commandLine;
    char* currOut = _buffer;
    char term = ' ';
    bool sub = false;

    enum ParserState
    {
        SkipWhitespace,
        SetTerm,
        Copy,
        Escape,
        End,
    };

    ParserState state = SkipWhitespace;

    while ('\0' != *curr
    &&     _term != *curr
    &&     argc < _maxArgvs)
    {
        switch (state)
        {
            case SkipWhitespace:
                for (; isspace(*curr); ++curr) {}; // skip whitespace
                state = SetTerm;
                break;

            case SetTerm:
                if ('"' == *curr)
                {
                    term = '"';
                    ++curr; // skip begining quote
                }
                else
                {
                    term = ' ';
                }

                _argv[argc] = currOut;
                ++argc;

                state = Copy;
                break;

            case Copy:
                if ('\\' == *curr)
                {
                    state = Escape;
                }
                else if ('"' == *curr
                     &&  '"' != term)
                {
                    sub = !sub;
                }
                else if (isspace(*curr) && !sub)
                {
                    state = End;
                }
                else if (term != *curr || sub)
                {
                    *currOut = *curr;
                    ++currOut;
                }
                else
                {
                    state = End;
                }
                ++curr;
                break;

            case Escape:
                {
                    const char* start = --curr;
                    for (; '\\' == *curr; ++curr) {};

                    if ('"' != *curr)
                    {
                        int count = (int)(curr-start);

                        curr = start;
                        for (int ii = 0; ii < count; ++ii)
                        {
                            *currOut = *curr;
                            ++currOut;
                            ++curr;
                        }
                    }
                    else
                    {
                        curr = start+1;
                        *currOut = *curr;
                        ++currOut;
                        ++curr;
                    }
                }
                state = Copy;
                break;

            case End:
                *currOut = '\0';
                ++currOut;
                state = SkipWhitespace;
                break;
        }
    }

    *currOut = '\0';
    if (0 < argc
    &&  '\0' == _argv[argc-1][0])
    {
        --argc;
    }

    _bufferSize = (uint32_t)(currOut - _buffer);
    _argc = argc;

    if ('\0' != *curr)
    {
        ++curr;
    }

    return curr;
}

#endif //CMFT_TOKENIZE_H_HEADER_GUARD

/* vim: set sw=4 ts=4 expandtab: */
