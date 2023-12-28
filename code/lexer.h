#pragma once

#include "common.h"

#define MAX_TOKEN_SIZE 512
#define TOTAL_TOKEN_COUNT 8

struct String
{
    u64 length = 0;
    char *data = null;
};

enum TokenType
{
    // @note first 255 tokens are ascii characters (see ASCII table)
    // so it's easier to use them directly
    TokenType_IDENTIFIER = 256,
    TokenType_NUMBER,
    TokenType_STRING,

    TokenType_PLUS_EQUALS, // +=
    TokenType_MINUS_EQUALS, // -=
    TokenType_TIMES_EQUALS, // *=
    TokenType_DIV_EQUALS, // /=
    TokenType_MOD_EQUALS, // %=
    TokenType_IS_EQUAL, // ==
    TokenType_IS_NOT_EQUAL, // !=
    TokenType_LESS_EQUALS, // <=
    TokenType_GREATER_EQUALS, // >=

    TokenType_LOGICAL_AND, // && :: and
    TokenType_LOGICAL_OR, // || :: or

    TokenType_BINARY_XOR, // xor
    TokenType_BINARY_AND_EQUALS, // &=
    TokenType_BINARY_OR_EQUALS, // |=

    TokenType_SHIFT_LEFT, // <<
    TokenType_SHIFT_RIGHT, // >>
    TokenType_SHIFT_LEFT_EQUALS, // <<=
    TokenType_SHIFT_RIGHT_EQUALS, // >>=

    TokenType_DOUBLE_DOT, // ..
    TokenType_RIGHT_ARROW, // ->

    TokenType_RESERVED_TYPE,

    __TokenType_FIRST_KEYWORD,
        TokenType_KEYWORD_ALIAS = __TokenType_FIRST_KEYWORD,
        TokenType_KEYWORD_AS,
        TokenType_KEYWORD_AUTO_CAST,
        
        TokenType_KEYWORD_BREAK,
        
        TokenType_KEYWORD_CASE,
        TokenType_KEYWORD_CAST,
        TokenType_KEYWORD_CONST,
        TokenType_KEYWORD_CONTINUE,
        
        TokenType_KEYWORD_DEFER,
        
        TokenType_KEYWORD_ELSE,
        TokenType_KEYWORD_ENUM,
        TokenType_KEYWORD_EXTERN,
        
        TokenType_KEYWORD_FALSE,
        TokenType_KEYWORD_FOR,
        TokenType_KEYWORD_FUNCTION,

        TokenType_KEYWORD_IF,
        TokenType_KEYWORD_INLINE,
        
        TokenType_KEYWORD_NO_INLINE,
        TokenType_KEYWORD_NULL,

        TokenType_KEYWORD_OPERATOR,
        
        TokenType_KEYWORD_RETURN,
        
        TokenType_KEYWORD_STRUCT,
        TokenType_KEYWORD_SWITCH,
        TokenType_KEYWORD_SIZE_OF,
        
        TokenType_KEYWORD_THEN,
        TokenType_KEYWORD_TRUE,
        TokenType_KEYWORD_TYPE,
        
        TokenType_KEYWORD_UNDEFINED,
        TokenType_KEYWORD_UNION,
        TokenType_KEYWORD_USING,
        
        TokenType_KEYWORD_WHILE,
        TokenType_KEYWORD_WITH,
    __TokenType_LAST_KEYWORD = TokenType_KEYWORD_WITH,

    TokenType_END_OF_FILE,
    TokenType_ERROR
};

struct Token
{
    TokenType type = TokenType_ERROR;
    String name;

    int line_start = 0;
    int col_start  = 0;
    int line_end   = 0;
    int col_end    = 0;

    int flags = 0;
};

struct Lexer
{
    String input;
    u64 input_cursor = 0;

    int current_line_number     = 0;
    int current_character_index = 0;
    int total_lines_processed   = 0;
    int last_line_number = 0;

    char token_buffer[MAX_TOKEN_SIZE];
    Token tokens[TOTAL_TOKEN_COUNT];
    int token_cursor = 0;
    int number_of_tokens = 0;
    Token eof;
    
    b8 should_stop_processing = false;

    b8 initialize(String source);
    Token *peek_next_token(void);
    Token *peek_token(int index);
    Token *generate_token(void);
    void eat_token(void);
    Token *get_unused_token(void);

    void eat_character(void);
    int peek_next_character(void);
    void unwind_one_character(void);

    void set_token_position(Token *token);
    void set_token_end(Token *token);
    void eat_until_new_line(void);
    void eat_block_comment(void);

    Token *make_one_character_token(int type);
    Token *check_for_equals(int token, int composed_token, b8 should_consume, int subtract_amount = 0);
    Token *make_identifier(void);
    Token *make_number(void);
    Token *make_binary_number(void);
    Token *make_hex_number(void);
    Token *make_string(void);

    int parse_decimal_digit(void);
    int parse_hexadecimal_digit(void);
    TokenType check_for_keyword(Token *token);

    void report_error(Token *pos, const char *format, ...);
};

enum LiteralNumber
{
    LiteralNumber_UNKNOWN     = 0,
    LiteralNumber_BINARY      = 0x1,
    LiteralNumber_HEXADECIMAL = 0x2,
    LiteralNumber_FLOAT       = 0x4,
};

// @debug
const char *token_type_strings(TokenType type);