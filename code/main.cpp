#include "lexer.h"

#include <stdio.h>

int main(void)
{
    String input;
    char source_code_memory[] = R"(
    /**
     * square function:
     * x: float
     * returns float
    */
    function square(x: float): float
    {
        return x * x;
    }

    // main entry point
    function main()
    {
        result: int = cast(int)square( cast(float) 2 );
        print(result);
    }
    )";
    input.length = sizeof(source_code_memory);
    input.data = source_code_memory;

    Lexer lexer;
    if (!lexer.initialize(input)) return -1;

    while (true)
    {
        Token t = lexer.peek_next_token();
        if (lexer.should_stop_processing) return -1;
        if (t.type == TokenType_END_OF_FILE) break;

        if (t.type < 256)
        {
            printf("Token(%d, %d) %c\n", t.line_start, t.col_start, t.type);
        }
        else if (t.type >= __TokenType_FIRST_KEYWORD && t.type <= __TokenType_LAST_KEYWORD)
        {
            printf("Keyword(%d, %d) '%s', length: %d\n", t.line_start, t.col_start, t.name.data, (int)t.name.length);
        }
        else if (t.type == TokenType_RESERVED_TYPE)
        {
            printf("ReservedType(%d, %d) '%s', length: %d\n", t.line_start, t.col_start, t.name.data, (int)t.name.length);
        }
        else
        {
            printf("Token(%d, %d) '%s', length: %d\n", t.line_start, t.col_start, t.name.data, (int)t.name.length);
        }
    }

    printf("\nLexer:\nTotal lines processed: %d\n", lexer.total_lines_processed);

    return 0;
}