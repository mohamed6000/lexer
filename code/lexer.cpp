#include "lexer.h"
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

b8 strings_match(const char *a, const char *b)
{
    if (!a) { if (!b) return true; else return false; }
    while (*a)
    {
        if (*a != *b) return false;
        ++a;
        ++b;
    }
    return !(*b);
}

inline b8 starts_identifier(int c)
{
    if (isalpha(c) || c == '_') return true;
    return false;
}

inline b8 continus_identifier(int c)
{
    if (isalnum(c) || c == '_') return true;
    return false;
}

/////////////////////////////////////////////////////////
b8 Lexer::initialize(String source)
{
    input = source;
    current_line_number = 1;
    current_character_index = 1;
    return true;
}

Token Lexer::peek_next_token(void)
{
    while (true)
    {
        int c = peek_next_character();
        while (isspace(c))
        {
            eat_character();
            c = peek_next_character();
        }
        if ((c == -1) || (c == 0))
        {
            // end of file token
            return make_one_character_token(TokenType_END_OF_FILE);
        }
        if (starts_identifier(c))
        {
            return make_identifier();
        }
        if (isdigit(c))
        {
            return make_number();
        }
        if (c == '.')
        {
            eat_character();
            c = peek_next_character();
            if (c == '.')
            {
                Token result = make_one_character_token(TokenType_DOUBLE_DOT);
                eat_character();
                return result;
            }
            // floating point
            if (isdigit(c))
            {
                unwind_one_character();
                return make_number();
            }
            
            return make_one_character_token('.');
        }

        switch (c)
        {
            case '=': return check_for_equals(c, TokenType_IS_EQUAL, true);
            case '+': return check_for_equals(c, TokenType_PLUS_EQUALS, true);
            case '*': return check_for_equals(c, TokenType_TIMES_EQUALS, true);
            case '%': return check_for_equals(c, TokenType_MOD_EQUALS, true);
            case '!': return check_for_equals(c, TokenType_IS_NOT_EQUAL, true);

            case '/':
            {
                eat_character();
                c = peek_next_character();
                if (c == '/') // single line comment
                {
                    eat_character();
                    eat_until_new_line();
                    continue;
                }
                else if (c == '*') // multi line comment
                {
                    eat_character();
                    eat_block_comment();
                    continue;
                }
                else
                {
                    return check_for_equals('/', TokenType_DIV_EQUALS, false);
                }
            } break;

            case '-':
            {
                eat_character();
                int next = peek_next_character();
                if (next == '>')
                {
                    // @hack: in this case we eat the character after making the token
                    // because it's combined of two characters '->'
                    Token result = make_one_character_token(TokenType_RIGHT_ARROW);
                    eat_character();
                    return result;
                }
                else
                {
                    return check_for_equals('-', TokenType_MINUS_EQUALS, false);
                }
            } break;

            case '<':
            {
                eat_character();
                int next = peek_next_character();
                if (next == '<')
                {   // << or <<=
                    return check_for_equals(TokenType_SHIFT_LEFT, TokenType_SHIFT_LEFT_EQUALS, true, 1);
                }
                else
                {   // < or <=
                    return check_for_equals('<', TokenType_LESS_EQUALS, false);
                }
            } break;

            case '>':
            {
                eat_character();
                int next = peek_next_character();
                if (next == '>')
                {   // >> or >>=
                    return check_for_equals(TokenType_SHIFT_RIGHT, TokenType_SHIFT_RIGHT_EQUALS, true, 1);
                }
                else
                {   // > or >=
                    return check_for_equals('>', TokenType_GREATER_EQUALS, false);
                }
            } break;

            case '&':
            {
                eat_character();
                int next = peek_next_character();
                if (next == '&')
                {
                    Token result = make_one_character_token(TokenType_LOGICAL_AND);
                    eat_character();
                    return result;
                }
                else
                {
                    return check_for_equals('&', TokenType_BINARY_AND_EQUALS, false);
                }
            } break;

            case '|':
            {
                eat_character();
                int next = peek_next_character();
                if (next == '|')
                {
                    Token result = make_one_character_token(TokenType_LOGICAL_OR);
                    eat_character();
                    return result;
                }
                else
                {
                    return check_for_equals('|', TokenType_BINARY_OR_EQUALS, false);
                }
            } break;

            case '"': return make_string();

            case '(':
            case ')':
            case '{':
            case '}':
            case '[':
            case ']':
            case ':':
            case ';':
            case ',':
            case '#':
            case '^': // '^' for pointers only (not used for bitwise xor operations)
            case '~':
            case '?':
            case '$':
            case '@':
            default:
            {
                eat_character();
                return make_one_character_token(c);
            } break;
        }
    }
}

void Lexer::eat_character(void)
{
    if (input.data[input_cursor] == '\n')
    {
        last_line_number = current_line_number;
        ++current_line_number;
        ++total_lines_processed;
        current_character_index = 0;
    }
    ++input_cursor;
    ++current_character_index;
}

int Lexer::peek_next_character(void)
{
    if (input_cursor >= input.length)
    {
        return -1;
    }
    return input.data[input_cursor];
}

void Lexer::unwind_one_character(void)
{
    assert(input_cursor > 0);
    --input_cursor;
    --current_character_index;
}

void Lexer::set_token_position(Token *token)
{
    token->line_start = current_line_number;
    token->col_start  = current_character_index;
}

void Lexer::set_token_end(Token *token)
{
    token->line_end = current_line_number;
    token->col_end  = current_character_index;
}

void Lexer::eat_until_new_line(void)
{
    int c;
    while (true)
    {
        c = peek_next_character();
        if (c == '\n') break;
        if (c == -1) break;
        if (c == 0) break;
        eat_character();
    }
}

void Lexer::eat_block_comment(void)
{
    int c;
    while (true)
    {
        c = peek_next_character();
        if (c == -1)
        {
            set_token_end(&eof);
            report_error(&eof, "Reached end of file from within a comment.");
            return;
        }
        // check for end of comment block
        if (c == '*')
        {
            eat_character();
            int next = peek_next_character();
            if (next == '/')
            {
                eat_character();
                break;
            }
        }
        else
        {
            eat_character();
        }
    }
}

Token Lexer::make_one_character_token(int type)
{
    Token result;
    set_token_position(&result);
    result.type = (TokenType)type;
    result.col_start -= 1;
    set_token_end(&result);
    return result;
}

Token Lexer::check_for_equals(int token, int composed_token, b8 should_consume, int subtract_amount)
{
    Token result;
    if (should_consume) eat_character();

    int next = peek_next_character();
    if (next == '=')
    {
        eat_character();
        result = make_one_character_token(composed_token);
        result.col_start -= subtract_amount + 1;
    }
    else
    {
        result = make_one_character_token(token);
        result.col_start -= subtract_amount;
    }
    return result;
}

Token Lexer::make_identifier(void)
{
    Token result;
    result.type = TokenType_IDENTIFIER;
    set_token_position(&result);
    
    int c = peek_next_character();
    char *cur = token_buffer;
    while (continus_identifier(c))
    {
        *cur++ = c;
        eat_character();
        c = peek_next_character();
    }
    *cur = 0;
    
    result.name.length = cur - token_buffer;
    result.name.data = token_buffer;
    result.type = check_for_keyword(&result);

    set_token_end(&result);
    return result;
}

Token Lexer::make_number(void)
{
    Token result;
    result.type = TokenType_NUMBER;
    set_token_position(&result);

    char *mantisse_cursor = null;
    char *exponent_cursor = null;

    char *cur = token_buffer;
    int c;
    
    while (true)
    {
        if (cur >= (token_buffer + MAX_TOKEN_SIZE)) break;
        c = peek_next_character();

        if ((cur == (token_buffer + 1)) && (*token_buffer == '0') && (result.flags == 0))
        {
            if ((c == 'b') || (c == 'B'))
            {
                cur = token_buffer;
                result.flags |= LiteralNumber_BINARY;
                eat_character();
                continue;
            }
            else if ((c == 'x') || (c == 'X'))
            {
                cur = token_buffer;
                result.flags |= LiteralNumber_HEXADECIMAL;
                eat_character();
                continue;
            }
        }

        if (c == '_')
        {
            eat_character();
            continue;
        }

        if (c == '.')
        {
            eat_character();
            c = peek_next_character();
            if (c == '.') // ..
            {
                unwind_one_character();
                break;
            }
            else
            {
                if (mantisse_cursor)
                {
                    set_token_end(&result);
                    report_error(&result, "Can't have two decimal points in a number");
                    break;
                }

                if (result.flags & LiteralNumber_BINARY)
                {
                    set_token_end(&result);
                    report_error(&result, "Can't have a decimal point in a binary number");
                }

                if (result.flags & LiteralNumber_HEXADECIMAL)
                {
                    set_token_end(&result);
                    report_error(&result, "Can't have a decimal point in a hexadecimal number");
                }

                *cur++ = '.';
                mantisse_cursor = cur;
                result.flags |= LiteralNumber_FLOAT;
                continue;
            }
        }
        else
        {
            if (mantisse_cursor)
            {
                // float
                if (!isdigit(c))
                {
                    if ((c == 'e') || (c == 'E'))
                    {
                        if (exponent_cursor)
                        {
                            set_token_end(&result);
                            report_error(&result, "Can't have two exponents in a number");
                            break;
                        }

                        exponent_cursor = cur;
                        *cur++ = 'e';

                        eat_character();
                        c = peek_next_character();
                        if ((c == '+') || (c == '-'))
                        {
                            if (cur < (token_buffer + MAX_TOKEN_SIZE))
                            {
                                *cur++ = c;
                                eat_character();
                            }
                            continue;
                        }
                        else if (isdigit(c))
                        {
                            continue;
                        }
                        else
                        {
                            set_token_end(&result);
                            report_error(&result, "'e' in float literals must be followed by '+' or '-' or a numerical digit");
                            break;
                        }
                    }
                    if (c == 'f')
                    {
                        // literal ends with the 'f' postfix
                        eat_character();
                        *cur++ = 'f';
                        break;
                    }
                    // exit if the character is not a digit, 'e' or 'E'
                    // because we probably reached the end of the number
                    break;
                }
            }
            else
            {
                // integer
                // exit if the character is not a digit or a hex digit
                // because we probably reached the end of the number
                b8 should_exit = true;
                // we set digit 0 every time to avoid getting invalid digit in
                // binary number when we reach a ';' or something else
                int digit = 0;

                if (isdigit(c))
                {
                    digit = c - '0';
                    should_exit = false;
                }

                if (result.flags & LiteralNumber_BINARY)
                {
                    if (digit > 1)
                    {
                        should_exit = true;
                        set_token_end(&result);
                        report_error(&result, "Invalid digit in a binary number");
                        break;
                    }
                }
                else if (result.flags & LiteralNumber_HEXADECIMAL)
                {
                    if (c >= 'A' && c <= 'F')
                    {
                        should_exit = false;
                    }

                    if (c >= 'a' && c <= 'f')
                    {
                        should_exit = false;
                    }
                }

                if (should_exit)
                {
                    break;
                }
            }
        }

        *cur++ = c;
        eat_character();
    }
    *cur = 0;

    result.name.length = cur - token_buffer;
    result.name.data = token_buffer;
    set_token_end(&result);
    return result;
}

Token Lexer::make_string(void)
{
    Token result;
    result.type = TokenType_STRING;
    set_token_position(&result);
    eat_character();

    char *cur = token_buffer;

    while (true)
    {
        int c = peek_next_character();
        eat_character();

        if (c == '"') break;

        if (c == -1)
        {
            set_token_end(&result);
            report_error(&result, "Reached end of file within a string literal.");
            break;
        }

        if (c == '\n')
        {
            set_token_end(&result);
            report_error(&result, "Reached new line within a string literal.");
            break;
        }

        if (c == '\\')
        {
            int next = peek_next_character();
            if (next == 'a')
            {
                c = '\a';
                eat_character();
            }
            else if (next == 'd')
            {
                // parse 1 byte decimal integer
                eat_character();
                int high = parse_decimal_digit();
                if (high >= 0)
                {
                    int middle = parse_decimal_digit();
                    if (middle >= 0)
                    {
                        int low = parse_decimal_digit();
                        if (low < 0) low = 0;
                        c = low + (middle * 10) + (high * 100);
                        if (c > 255)
                        {
                            set_token_end(&result);
                            report_error(&result, "Decimal value of '%d' exceeds the limit.", c);
                        }
                    }
                }
            }
            else if (next == 'e')
            {
                // escape (esc) sequence
                eat_character();
                c = 0x1B;
            }
            else if (next == 'f')
            {
                eat_character();
                c = '\f';
            }
            else if (next == 'n')
            {
                eat_character();
                c = '\n';
            }
            else if (next == 'r')
            {
                eat_character();
                c = '\r';
            }
            else if (next == 't')
            {
                eat_character();
                c = '\t';
            }
            else if (next == 'v')
            {
                eat_character();
                c = '\v';
            }
            else if (next == 'x')
            {
                // parse 1 byte hexadecimal integer
                eat_character();
                int high = parse_hexadecimal_digit();
                if (high >= 0)
                {
                    int low = parse_hexadecimal_digit();
                    if (low >= 0)
                    {
                        c = low + (high * 16);
                    }
                }
            }
            else if (next == '0')
            {
                eat_character();
                c = '\0';
            }
            else if (next == '"')
            {
                eat_character();
                c = '\"';
            }
            else if (next == '\'')
            {
                eat_character();
                c = '\'';
            }
            else if (next == '\\')
            {
                eat_character();
                c = '\\';
            }
            else
            {
                set_token_end(&result);
                report_error(&result, "Unknown escape sequence '\\%c' in string literal.", next);
                c = next;
                eat_character();
            }
        }

        *cur++ = c;
    }
    
    *cur = 0;
    
    result.name.length = cur - token_buffer;
    if (result.name.length)
    {
        result.name.data = token_buffer;
    }
    else
    {
        // empty string
        result.name.data = null;
    }
    set_token_end(&result);
    return result;
}

int Lexer::parse_decimal_digit(void)
{
    int c = peek_next_character();

    if (c >= '0' && c <= '9')
    {
        eat_character();
        return c - '0';
    }

    report_error(&eof, "Invalid decimal digit.\n\\d must be followed by 3 decimal digits.");
    return -1;
}

int Lexer::parse_hexadecimal_digit(void)
{
    int c = peek_next_character();

    if (c >= '0' && c <= '9')
    {
        eat_character();
        return c - '0';
    }
    else if (c >= 'A' && c <= 'F')
    {
        eat_character();
        return 10 + (c - 'A');
    }
    else if (c >= 'a' && c <= 'f')
    {
        eat_character();
        return 10 + (c - 'a');
    }

    report_error(&eof, "Invalid hexadecimal digit.\n\\x must be followed by 2 hexadecimal digits.");
    return -1;
}

TokenType Lexer::check_for_keyword(Token *token)
{
    char *name = token->name.data;
    u64 length = token->name.length;

    switch (length)
    {
        case 2:
        {
            if (strings_match(name, "as")) return TokenType_KEYWORD_AS;
            if (strings_match(name, "if")) return TokenType_KEYWORD_IF;
            if (strings_match(name, "or")) return TokenType_LOGICAL_OR;
            // reserved types
            if (strings_match(name, "s8")) return TokenType_RESERVED_TYPE;
            if (strings_match(name, "u8")) return TokenType_RESERVED_TYPE;
        } break;
        case 3:
        {
            if (strings_match(name, "for")) return TokenType_KEYWORD_FOR;
            if (strings_match(name, "and")) return TokenType_LOGICAL_AND;
            if (strings_match(name, "xor")) return TokenType_BINARY_XOR;
            // reserved types
            if (strings_match(name, "int")) return TokenType_RESERVED_TYPE;
            if (strings_match(name, "s16")) return TokenType_RESERVED_TYPE;
            if (strings_match(name, "s32")) return TokenType_RESERVED_TYPE;
            if (strings_match(name, "s64")) return TokenType_RESERVED_TYPE;
            if (strings_match(name, "u16")) return TokenType_RESERVED_TYPE;
            if (strings_match(name, "u32")) return TokenType_RESERVED_TYPE;
            if (strings_match(name, "u64")) return TokenType_RESERVED_TYPE;
            if (strings_match(name, "f32")) return TokenType_RESERVED_TYPE;
            if (strings_match(name, "f64")) return TokenType_RESERVED_TYPE;
        } break;
        case 4:
        {
            if (strings_match(name, "case")) return TokenType_KEYWORD_CASE;
            if (strings_match(name, "cast")) return TokenType_KEYWORD_CAST;
            if (strings_match(name, "else")) return TokenType_KEYWORD_ELSE;
            if (strings_match(name, "enum")) return TokenType_KEYWORD_ENUM;
            if (strings_match(name, "null")) return TokenType_KEYWORD_NULL;
            if (strings_match(name, "then")) return TokenType_KEYWORD_THEN;
            if (strings_match(name, "true")) return TokenType_KEYWORD_TRUE;
            if (strings_match(name, "type")) return TokenType_KEYWORD_TYPE;
            if (strings_match(name, "with")) return TokenType_KEYWORD_WITH;
            // reserved types
            if (strings_match(name, "bool")) return TokenType_RESERVED_TYPE;
            if (strings_match(name, "void")) return TokenType_RESERVED_TYPE;
        } break;
        case 5:
        {
            if (strings_match(name, "alias")) return TokenType_KEYWORD_ALIAS;
            if (strings_match(name, "break")) return TokenType_KEYWORD_BREAK;
            if (strings_match(name, "const")) return TokenType_KEYWORD_CONST;
            if (strings_match(name, "defer")) return TokenType_KEYWORD_DEFER;
            if (strings_match(name, "false")) return TokenType_KEYWORD_FALSE;
            if (strings_match(name, "union")) return TokenType_KEYWORD_UNION;
            if (strings_match(name, "using")) return TokenType_KEYWORD_USING;
            if (strings_match(name, "while")) return TokenType_KEYWORD_WHILE;
            // reserved types
            if (strings_match(name, "float")) return TokenType_RESERVED_TYPE;
        } break;
        case 6:
        {
            if (strings_match(name, "extern")) return TokenType_KEYWORD_EXTERN;
            if (strings_match(name, "inline")) return TokenType_KEYWORD_INLINE;
            if (strings_match(name, "return")) return TokenType_KEYWORD_RETURN;
            if (strings_match(name, "struct")) return TokenType_KEYWORD_STRUCT;
            if (strings_match(name, "switch")) return TokenType_KEYWORD_SWITCH;
            // reserved types
            if (strings_match(name, "string")) return TokenType_RESERVED_TYPE;
        } break;
        case 7:
        {
            if (strings_match(name, "size_of")) return TokenType_KEYWORD_SIZE_OF;
        } break;
        case 8:
        {
            if (strings_match(name, "continue")) return TokenType_KEYWORD_CONTINUE;
            if (strings_match(name, "function")) return TokenType_KEYWORD_FUNCTION;
            if (strings_match(name, "operator")) return TokenType_KEYWORD_OPERATOR;
        } break;
        case 9:
        {
            if (strings_match(name, "auto_cast")) return TokenType_KEYWORD_AUTO_CAST;
            if (strings_match(name, "no_inline")) return TokenType_KEYWORD_NO_INLINE;
            if (strings_match(name, "undefined")) return TokenType_KEYWORD_UNDEFINED;
        } break;
    }

    return TokenType_IDENTIFIER;
}

void Lexer::report_error(Token *pos, const char *format, ...)
{
    should_stop_processing = true;

    fprintf(stderr, "<filename>:%d:%d: Error: ", pos->line_start, pos->col_start);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputc('\n', stderr);
}