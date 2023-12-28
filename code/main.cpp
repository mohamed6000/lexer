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
        Token *t = lexer.peek_next_token();
        if (t->type == TokenType_END_OF_FILE) break;

        lexer.eat_token();

        if (t->type < 128)
        {
            char data[1];
            data[0] = t->type;
            String s;
            s.length = 1;
            s.data = (char*)data;
            fprintf(stdout, "%d,%d: '%.*s'\n", t->line_start, t->col_start, (int)s.length, s.data);
        }
        else
        {
            switch (t->type)
            {
                case TokenType_IDENTIFIER:
                    fprintf(stdout, "%d,%d: IDENTIFIER '%.*s'\n", t->line_start, t->col_start, (int)t->name.length, t->name.data);
                    break;
                default:
                    fprintf(stdout, "%d,%d: %s\n", t->line_start, t->col_start, token_type_strings(t->type));
                    break;
            }
        }
    }

    printf("\nLexer:\nTotal lines processed: %d\n", lexer.total_lines_processed);

    return 0;
}