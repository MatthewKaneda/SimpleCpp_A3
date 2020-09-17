/**
 * Scanner class for a simple interpreter.
 *
 * (c) 2020 by Ronald Mak
 * Department of Computer Science
 * San Jose State University
 */
#ifndef SCANNER_H_
#define SCANNER_H_

#include "Source.h"
#include "Token.h"

namespace frontend {

using namespace std;

class Scanner
{
private:
    Source *source;

public:
    /**
     * Constructor.
     * @param source the input source.
     */
    Scanner(Source *source) : source(source) {}

    /**
     * Extract the next token from the source.
     * @return the token.
     */
    Token *nextToken()
    {
        // Skip blanks, comments, and other whitespace characters.
        char ch = nextNonblankCharacter();

        if (isalpha(ch))      return Token::Word(ch, source);
        else if (isdigit(ch)) return Token::Number(ch, source);
        else if (ch == '\'')  return Token::CharacterOrString(ch, source);
        else                  return Token::SpecialSymbol(ch, source);
    }

private:
    /**
     * Skip blanks, comments, and other whitespace characters
     * and return the next nonblank character.
     * @return the next nonblank character.
     */
    char nextNonblankCharacter()
    {
        char ch = source->currentChar();

        while ((ch == '{') || isspace(ch))
        {
            if (ch == '{')
            {
                // Consume characters of the comment.
                while ((ch != '}') && (ch != EOF)) ch = source->nextChar();
            }

            ch = source->nextChar();  // consume character
        }

        return ch;  // nonblank character
    }
};

}  // namespace frontend

#endif /* SCANNER_H_ */
