#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define TokenList                 \
    TokenEntry(INVALID)           \
    TokenEntry(IDENTIFIER)        \
    TokenEntry(WHITESPACE)        \
	TokenEntry(NEWLINE)			  \
                             	  \
    TokenEntry(PAREN_OPEN)        \
    TokenEntry(PAREN_CLOSE)       \
    TokenEntry(BRACE_OPEN)        \
    TokenEntry(BRACE_CLOSE)       \
                                  \
    TokenEntry(COMMA)             \
    TokenEntry(QUOTE)             \
	TokenEntry(SLASH)			  \
                                  \
    TokenEntry(POUND)             \
    TokenEntry(POUND_FOR)         \
    TokenEntry(POUND_ENDFOR)      \
    TokenEntry(POUND_LINE)        \
    TokenEntry(POUND_LINE_CLIP)   \
	TokenEntry(POUND_WORD)		  \
    TokenEntry(POUND_REPLACE)     \
                                  \
    TokenEntry(STRING)            \
    TokenEntry(COMMENT)           \
    TokenEntry(INTEGER)           \
    TokenEntry(FLOAT)             \
								  \
	TokenEntry(END_OF_BUFFER)	  \


#if 0
#define ProcedureList             \
    ProcedureEntry(r, "target replacewith", "Replace", "Replaces the target string with the replace string.  Searches all text within the current scope")        \
    ProcedureEntry(rw, "target replace_with", "Replace Word", "Replaces the target word with the replace word.  Searches all text within the current scope")    \
    ProcedureEntry(d, "target", "Delete", "Removes the target text from all text within the current scope")                                                        \
    ProcedureEntry(dw, "target", "Delete Word", "Deletes the current word from all the text within the current scope")                                            \
    ProcedureEntry(ptr, "target", "To Pointer", "Converts the targets syntax to treat it as if it were a pointer")                                                \
    ProcedureEntry(val, "target", "To Value", "Converts the targets syntax to treat it as if it were a value")
#endif

typedef enum
{
#define TokenEntry(name) TokenType_##name,
    TokenList
    TokenType_COUNT
#undef TokenEntry
} TokenType;

static const char *TOKEN_STRINGS[] =
{
#define TokenEntry(name) #name,
    TokenList
#undef TokenEntry
};

typedef struct {
    TokenType type;
    const char *text;
    uint32_t length;
} Token;

typedef struct {
    size_t buffer_size;
    const char *buffer;
    const char *current;
    uint32_t line_number;
    uint32_t line_offset;
    bool is_first_token_in_line;
    Token token;
} Lexer;

typedef struct
EditResultStruct {
    char *buffer;
    size_t size;
    struct EditResultStruct *next;
} EditResult;


static EditResult *g_root_edit_result = NULL;
static EditResult *g_last_edit_result = NULL;
static const char *buffer_read_pos;
static inline void push_edit_result(const char *buffer, size_t size)
{
    EditResult *result = (EditResult*)malloc(sizeof(EditResult) + size);
    result->buffer = (char *)(result + 1);
    memcpy(result->buffer, buffer, size);
    result->size = size;
    result->next = NULL;

    if (g_root_edit_result == NULL) {
        g_root_edit_result = result;
        g_last_edit_result = result;
    } else {
        g_last_edit_result->next = result;
        g_last_edit_result = result;
    }
}

static inline bool is_alpha(char c)
{
    bool result = ((c >= 'A') && (c <= 'Z')) ||
        ((c >= 'a') && (c <= 'z'));
    return result;
}

static inline bool is_number(char c)
{
    bool result = ((c >= '0') && (c <= '9'));
    return result;
}

static inline bool is_end_of_line(const char *cursor)
{
    if (*cursor == '\r')
    {
        if (cursor[1] == '\n')
        {
            return false;
        }
        return true;
    }

    if (*cursor == '\n')
    {
        return true;
    }

    return false;
}

static inline bool is_whitespace(const char c)
{
    bool result = (c == '\t' || c == ' ' || c == '\n' || c == '\r');
    return result;
}

static inline int strings_match(const char* string_a, size_t length_a, const char* string_b, size_t length_b)
{
    if (length_a != length_b) return 0;
    for (size_t i = 0; i < length_a; i++) {
        if (string_a[i] != string_b[i]) return 0;
    }
    return 1;
}

static inline int64_t to_int(Token token)
{
	assert(token.type == TokenType_INTEGER);
    int64_t result = 0;
    for (int i = token.length - 1; i >= 0; i--)
    {
        uint8_t digit_value = token.text[i] - '0';
        result += digit_value + (i * 10);
    }
    return result;
}

static void increment_past_whitespace(Lexer *lex)
{
    while (*lex->current == '\n' ||
                *lex->current == '\t' ||
                *lex->current == ' ' ||
                *lex->current == '\r')
    {
        if (*lex->current == '\r') {
            lex->current++;
            if (*lex->current != '\n') {
                lex->line_number++;
                lex->line_offset = 0;
                lex->is_first_token_in_line = true;
            }
        } else if (*lex->current == '\n') {
            lex->line_number++;
            lex->line_offset = 0;
            lex->is_first_token_in_line = true;
            lex->current++;
        } else {
            lex->current++;
        }
    }
}
static inline void lex_next_token_and_whitespace(Lexer *lex)
{
    lex->token.text = lex->current;

	if (*lex->current == '\r') 
	{
		lex->current++;
		if (*lex->current == '\n') 
		{
			lex->line_number++;
			lex->line_offset = 0;
			lex->current++;
			lex->token.type = TokenType_NEWLINE;
		}
		else
		{
			lex->line_number++;
			lex->line_offset = 0;
			lex->token.type = TokenType_NEWLINE;
		}
	} 
	else if (*lex->current == '\n') 
	{
		lex->line_number++;
		lex->line_offset = 0;
		lex->current++;
		lex->token.type = TokenType_NEWLINE;
	}

	else if (*lex->current == ' ' || *lex->current == '\t')
	{
		lex->current++;
		while (*lex->current == ' ' || *lex->current == '\t')
		{
			lex->current++;
		}
		lex->token.type = TokenType_WHITESPACE;
	}

    else if (is_alpha(*lex->current) || *lex->current == '_')
    {
        while (is_alpha(*lex->current) || is_number(*lex->current) || *lex->current == '_')
        {
            lex->current += 1;
        }
        lex->token.type = TokenType_IDENTIFIER;
    }

#define static_strlen(string) (sizeof(string) - 1)
#define matches_keyword(buffer, keyword) strings_match(buffer, static_strlen(keyword), keyword, static_strlen(keyword))

#define token_match(token_string) ((static_strlen(token_string) == 1) ? \
    *lex->current == *token_string : matches_keyword(lex->current, token_string))

#define check_token(token_type, token_string) else if (token_match(token_string)) { \
    lex->token.type = token_type; \
    lex->current += static_strlen(token_string); \
}

#define check_token_and_subtokens(token_type, token_string, subtoken_macro) else if (token_match(token_string)) { \
    lex->current += static_strlen(token_string); \
    if (0) {} subtoken_macro \
    else { \
        lex->token.type = token_type; \
    } \
}

    else if (is_number(*lex->current))
    {
        lex->token.text = lex->current;
        bool dot_was_parsed = false;
        while(is_number(*lex->current) || (!dot_was_parsed && *lex->current == '.'))
        {
            lex->current++;
        }
        lex->token.type = dot_was_parsed ? TokenType_FLOAT : TokenType_INTEGER;
    }

	else if (*lex->current == '/')
	{
		lex->current++;
		if (*lex->current == '/')
		{
			lex->token.type = TokenType_COMMENT;
			lex->current++;
		}
		else
		{
			lex->token.type = TokenType_SLASH;
		}
	}

    check_token(TokenType_PAREN_OPEN, "(")
    check_token(TokenType_PAREN_CLOSE, ")")
    check_token(TokenType_BRACE_OPEN, "{")
    check_token(TokenType_BRACE_CLOSE, "}")
    check_token(TokenType_COMMA, ",")
    check_token(TokenType_QUOTE, "\"")

    check_token_and_subtokens(TokenType_POUND, "#",
        check_token(TokenType_POUND_REPLACE, "r")
        check_token_and_subtokens(TokenType_POUND_LINE, "l",
            check_token(TokenType_POUND_LINE_CLIP, "c"))
        check_token(TokenType_POUND_FOR, "fl")
        check_token(TokenType_POUND_ENDFOR, "efl")
		check_token(TokenType_POUND_WORD, "w"))

    check_token(TokenType_END_OF_BUFFER, "\0")

    else
    {
        //TODO(Torin) After the lexer is complete
        //We should assert false here
        lex->token.type = TokenType_INVALID;
        lex->current++;
    }

	lex->token.length = lex->current - lex->token.text;
	lex->line_offset += lex->token.length;
}

static inline void lex_next_token(Lexer *lex)
{
    lex_next_token_and_whitespace(lex);
    bool is_first_token_in_line = lex->is_first_token_in_line;
    while(lex->token.type == TokenType_WHITESPACE)
    {
        lex_next_token_and_whitespace(lex);
    }
    lex->is_first_token_in_line = is_first_token_in_line;
}



static inline
void report_error_internal(Lexer *lex)
{
    printf("ERROR[%d:%d] ", lex->line_number + 1, lex->line_offset + 1);
}

#define report_error(lex_ptr, ...) report_error_internal(lex_ptr); printf(__VA_ARGS__); printf("\n")
#define report_error_and_exit(lex_ptr, ...) { report_error_internal(lex_ptr); printf(__VA_ARGS__); printf("\n"); abort(); }

static inline
void lex_and_expect_token(TokenType type, Lexer *lex)
{
    lex_next_token(lex);
    if (lex->token.type != type)
    {
        report_error(lex, "Expected Token '%s', instead parsed Token(%s)",
            TOKEN_STRINGS[type], TOKEN_STRINGS[lex->token.type]);
    }
}

static inline
void lex_and_require_valid_token(Lexer *lex)
{
    lex_next_token(lex);
    if (lex->token.type == TokenType_END_OF_BUFFER)
    {
        report_error(lex, "Reached end of file unexpectedly");
        abort();
    }
}

static inline
void ensure_token(Lexer *lex, TokenType type)
{
    if (lex->token.type != type)
    {
        report_error_and_exit(lex, "Expected token (%s) but instead found token (%s)",
                TOKEN_STRINGS[type], TOKEN_STRINGS[lex->token.type]);
    }
}

static inline
const char *seek_to_next_line(const char *cursor)
{
    while (!is_end_of_line(cursor))
    {
        cursor++;
    }
    cursor++;
    return cursor;
}

static inline void parse_block(Lexer *lex, const char *current_block)
{
    while (lex->token.type != TokenType_END_OF_BUFFER && lex->token.type != TokenType_BRACE_CLOSE)
    {
        lex_next_token(lex);

        if (lex->token.type == TokenType_BRACE_OPEN)
        {
            parse_block(lex, lex->token.text + 1);
            if (lex->token.type == TokenType_BRACE_CLOSE)
            {
                lex_next_token(lex);
            }
        }

        if (lex->token.type == TokenType_POUND_FOR)
        {
            size_t edit_offset = lex->token.text - buffer_read_pos;
            push_edit_result(buffer_read_pos, edit_offset);
            buffer_read_pos = lex->token.text + lex->token.length;
            buffer_read_pos = seek_to_next_line(buffer_read_pos);
            lex_and_require_valid_token(lex);

            size_t line_begin = lex->line_number;

			Token tokens[4096];
			size_t token_count = 0;
            while (lex->token.type != TokenType_POUND_ENDFOR)
            {
                lex_and_require_valid_token(lex);
				tokens[token_count++] = lex->token;
                if (lex->token.type == TokenType_POUND_FOR) 
				{
                    report_error_and_exit(lex, "Cannot have nested #for loops!  yet???")
                }
            }

            lex_and_expect_token(TokenType_PAREN_OPEN, lex);
            size_t line_count = lex->line_number - line_begin;
            lex_and_require_valid_token(lex);

            typedef enum {
                ProcedureType_TEXT,
                ProcedureType_LINE,
                ProcedureType_LINE_CLIP,
				ProcedureType_WORD,
            } ProcedureType;

            typedef struct {
                ProcedureType type;
                union
                {
                    struct //Text
                    {
                        const char *text;
                        size_t length;
                    };
                    struct //LineClip
                    {
                        size_t arg0;
                        size_t arg1;
                    };
					struct //Word
					{
						int wordIndex;
					};
                };
            } Procedure;

            Procedure procedures[128];
            size_t current_proc_count = 0;

#if 0
            const char *procedure_input = lex->token.text;
            int paren_level = 1;
            while (paren_level > 0)
            {
                lex_and_require_valid_token(lex);
                if (lex->token.type == TokenType_PAREN_OPEN)
                    paren_level++;
                else if (lex->token.type == TokenType_PAREN_CLOSE)
                    paren_level--;
            }
#endif

			auto parse_procedures = [](Lexer *lex)
			{
			};

            int paren_level = 1;
            while (paren_level > 0)
            {
                const char *current_text = lex->token.text;
                while (lex->token.type < TokenType_POUND || lex->token.type > TokenType_POUND_REPLACE)
                {
                    if (lex->token.type == TokenType_PAREN_OPEN)
                    {
                        paren_level++;
                    } else if (lex->token.type == TokenType_PAREN_CLOSE)
                    {
                        paren_level--;
                        if (paren_level == 0)
                        {
                            break;
                        }
                    }
                    lex_and_require_valid_token(lex);
                }

                Procedure *textProc = &procedures[current_proc_count++];
                textProc->type = ProcedureType_TEXT;
                textProc->text = current_text;
                textProc->length = lex->token.text - current_text;
                if (lex->token.type == TokenType_POUND_LINE)
                {
                    Procedure lineProc = {};
                    lineProc.type = ProcedureType_LINE;
                    procedures[current_proc_count++] = lineProc;
                } 
				else if (lex->token.type == TokenType_POUND_LINE_CLIP)
                {
                    Procedure *proc = &procedures[current_proc_count++];
                    proc->type = ProcedureType_LINE_CLIP;
                    lex_and_expect_token(TokenType_PAREN_OPEN, lex);
                    lex_and_expect_token(TokenType_INTEGER, lex);
                    proc->arg0 = to_int(lex->token);
                    lex_and_expect_token(TokenType_COMMA, lex);
                    lex_and_expect_token(TokenType_INTEGER, lex);
                    proc->arg1 = to_int(lex->token);
                    lex_and_expect_token(TokenType_PAREN_CLOSE, lex);
                }
				else if (lex->token.type == TokenType_POUND_WORD)
				{
					Procedure *proc = &procedures[current_proc_count++];
					proc->type = ProcedureType_WORD;
					lex_and_expect_token(TokenType_PAREN_OPEN, lex);
					lex_and_expect_token(TokenType_INTEGER, lex);
					proc->wordIndex = to_int(lex->token);
					lex_and_expect_token(TokenType_PAREN_CLOSE, lex);
				}
                lex_next_token(lex);
            }


			buffer_read_pos = lex->token.text;

#if 0
            const char *input_read_pos = procedure_input;
            size_t proc_input_length = (lex->token.text - procedure_input);
            for (size_t i = 0; i < proc_input_length; i++)
            {
                if (procedure_input[i] == '#')
                {
                    if (matches_keyword(&procedure_input[i + 1], "line"))
                    {
                        Procedure textProc = {};
                        textProc.type = ProcedureType_TEXT;
                        textProc.text = input_read_pos;
                        textProc.length = &procedure_input[i] - input_read_pos;
                        procedures[current_proc_count++] = textProc;
                        i += static_strlen("#line");
                        input_read_pos = &procedure_input[i];

                        if (matches_keyword(&procedure_input[i + 1 + static_strlen("line")])
                        {
                        } else
                        {
                        }
                    }
                }
            }

            size_t remaining_text_length = proc_input_length - (input_read_pos - procedure_input);
            if (remaining_text_length > 0)
            {
                Procedure proc = {};
                proc.type = ProcedureType_TEXT;
                proc.text = input_read_pos;
                proc.length = remaining_text_length;
                procedures[current_proc_count++] = proc;
            }

#endif
#define writebuffer(buffer, data, length) memcpy(buffer, data, length); buffer += length;
			char temp_buffer[4096];
			char *write_pos = temp_buffer;

			//XXX make sure token index is set to the first line in the for loop!

			Token *token = tokens;
			for (size_t i = 0; i < line_count; i++)
			{
				Token *first_token_in_line = token;
				if (token->type == TokenType_NEWLINE)
				{
					*write_pos++ = '\n';
					token++;
					continue;	
				}

				while (token->type == TokenType_WHITESPACE)
					token++;
				
				if (token->type == TokenType_COMMENT)
				{
					size_t write_length = token->text - first_token_in_line->text;
					memcpy(write_pos, first_token_in_line->text, write_length);
					write_pos += write_length;
					continue;
				}

				while (token->type != TokenType_NEWLINE)
					token++;

				for (size_t n = 0; n < current_proc_count; n++)
				{
					Procedure *proc = &procedures[n];
					switch (proc->type)
					{
					
						case ProcedureType_TEXT:
						{
							memcpy(write_pos, proc->text, proc->length);
							write_pos += proc->length;
						} break;
					
						case ProcedureType_LINE:
						{
							size_t write_size = (token->text + token->length) - first_token_in_line->text;
							memcpy(write_pos, first_token_in_line->text, write_size);
							write_pos += write_size;
						} break;

						case ProcedureType_LINE_CLIP:
						{
							size_t line_length = token->text - first_token_in_line->text;
							size_t write_size = line_length - proc->arg1 - proc->arg0;
							memcpy(write_pos, first_token_in_line->text, write_size);
							write_pos += write_size;
						} break;

						case ProcedureType_WORD:
						{
							Token *word_token = first_token_in_line;
							int word_index = -1;
							while (1)
							{
								while (word_token->type != TokenType_IDENTIFIER)
									word_token++;
								word_index += 1;
								if (word_index == proc->wordIndex)
								{
									break;
								}
								else
								{
									word_token++;
								}
							}
							memcpy(write_pos, word_token->text, word_token->length);
							write_pos += word_token->length;
						} break;
				}
			}

			token++;
			memcpy(write_pos, "\n", 1);
			write_pos += 1;
		}
		push_edit_result(temp_buffer, write_pos - temp_buffer);
	}
#if 0

#define writebuffer(buffer, data, length) memcpy(buffer, data, length); buffer += length;
			char temp_buffer[4096];
			char *write_pos = temp_buffer;
			const char *line_head = buffer_read_pos;
			const char *line_cursor = buffer_read_pos;
			const char *whitespace_end = line_head;
			buffer_read_pos = lex->token.text;
			
			for (size_t i = 0; i < line_count; i++)
            {
                bool line_is_comment = false;
                while (is_whitespace(*whitespace_end))
                {
                    if (is_end_of_line(whitespace_end))
                    {
                        line_count--;
                        if (i >= line_count) break;
                    }
                    whitespace_end++;
                }

                line_cursor = whitespace_end;
                while (!is_end_of_line(line_cursor))
                {
                    if (line_cursor[0] == '/' && line_cursor[1] == '/')
                    {
                        if (line_cursor == whitespace_end)
                        {
                            line_is_comment = true;
                        } else
                        {
                            //TODO(Torin)
                            //Comment and end of line should be ignored!
                        }
                    }
                    line_cursor++;
                }



                size_t whitespace_length = whitespace_end - line_head;
                size_t line_length = line_cursor - whitespace_end;
                memcpy(write_pos, line_head, whitespace_length);
                write_pos += whitespace_length;

                if (line_is_comment)
                {
                    writebuffer(write_pos, whitespace_end, line_length);
                } 


				else
                {
                    for (size_t n = 0; n < current_proc_count; n++)
                    {
                        Procedure *proc = &procedures[n];
                        switch (proc->type)
                        {
                        case ProcedureType_TEXT:
                        {
                            memcpy(write_pos, proc->text, proc->length);
                            write_pos += proc->length;
                        } break;
                        case ProcedureType_LINE:
                        {
                            memcpy(write_pos, whitespace_end, line_length);
                            write_pos += line_length;
                        } break;
                        case ProcedureType_LINE_CLIP:
                        {
                            writebuffer(write_pos, whitespace_end + proc->arg0, line_length - proc->arg1 - proc->arg0);
                        } break;
						case ProcedureType_WORD:
						{

							//NOTE(Torin) this is bad and will do bad things if it finds a number like 0xFF
							const char *word_begin = whitespace_end;
							int current_word_index = -1;
							while (current_word_index != proc->wordIndex)
							{
								if (is_alpha(*word_begin) || *word_begin == '_')
								{
									current_word_index++;
									if (current_word_index == proc->wordIndex)
									{
										break;
									}
									else
									{
										while (is_alpha(*word_begin) || is_number(*word_begin) || *word_begin == '_')
										{
											word_begin++;
										}
									}
								}
								else
								{
									word_begin++;
								}
							}

							size_t write_length = word_begin - whitespace_end;
							writebuffer(write_pos, word_begin, write_length);

						} break;
                        };
                    }


                }

                const char *next_line_head = seek_to_next_line(line_cursor);
                size_t new_line_count = 1;
                while (is_end_of_line(next_line_head))
                {
                    new_line_count++;
                    next_line_head = seek_to_next_line(next_line_head);
                }
                i += (new_line_count - 1);

                for (size_t n = 0; n < new_line_count; n++)
                {
                    memcpy(write_pos, "\n", 1);
                    write_pos++;
                }

                line_head = next_line_head;
                line_cursor = line_head;
                whitespace_end = line_cursor;
            }
#endif

        //@Replace
        else if (lex->token.type == TokenType_POUND_REPLACE)
        {
            size_t edit_size;
            if (current_block == NULL)
            {
                edit_size = buffer_read_pos - lex->buffer;
            }
            else
            {
                edit_size = current_block - buffer_read_pos;
            }


            push_edit_result(buffer_read_pos, edit_size);
            buffer_read_pos += edit_size;

            const char *skip_begin = lex->token.text;
            bool eat_replace_token = false;

            {
                const char *temp_seeker = lex->token.text;
                while (*temp_seeker != '\n')
                {
                    if (!is_whitespace(*temp_seeker) || temp_seeker == lex->token.text) {
                        eat_replace_token = true;
                        break;
                    }
                    temp_seeker--;
                }
            }

            lex_and_require_valid_token(lex);

            typedef struct {
                const char *text;
                size_t length;
            } String;

            String arg0, arg1;
            while(lex->token.type == TokenType_WHITESPACE)
                lex_and_require_valid_token(lex);
            ensure_token(lex, TokenType_IDENTIFIER);

            arg0.text = lex->token.text;
            arg0.length = lex->token.length;

            lex_and_require_valid_token(lex);
            while(lex->token.type == TokenType_WHITESPACE)
                lex_and_require_valid_token(lex);
            ensure_token(lex, TokenType_IDENTIFIER);

            arg1.text = lex->token.text;
            arg1.length = lex->token.length;

            {
                if (eat_replace_token == true)
                {
                    const char *temp_seeker = lex->token.text + lex->token.length;
                    while (*temp_seeker != '\n' && *temp_seeker != 0)
                    {
                        if (!is_whitespace(*temp_seeker))
                        {
                            eat_replace_token = false;
                            break;
                        }
                        temp_seeker++;
                    }
                }
            }

            const char *skip_end = lex->token.text;
            lex_next_token(lex);

            if (eat_replace_token)
            {
                skip_end = lex->token.text;
            }

            const char *search_begin;
            const char *search_end;
            if (current_block == NULL)
            {
                search_begin = lex->buffer;
                search_end = lex->buffer + lex->buffer_size;
            }
            else
            {
                search_begin = current_block;
                int brace_level = current_block != NULL;
                while (brace_level > 0)
                {
                    if (lex->token.type == TokenType_BRACE_OPEN)
                        brace_level++;
					else if (lex->token.type == TokenType_BRACE_CLOSE)
						brace_level--;
                    lex_next_token(lex);
                }

                search_end = lex->token.text + 1;
            }

            size_t match_read_pos = 0;
            auto find_and_replace = [&](const char *search_pos, const char *search_end,
                    String find, String replace)
            {
                while (search_pos != search_end)
                {
                    if (*search_pos == arg0.text[match_read_pos])
                    {
                        match_read_pos++;
                        if (match_read_pos == arg0.length)
                        {
                            size_t edit_size = (search_pos - arg0.length) - buffer_read_pos + 1;
                            push_edit_result(buffer_read_pos, edit_size);
                            buffer_read_pos += edit_size + find.length;
                            push_edit_result(replace.text, replace.length);
                            match_read_pos = 0;
                        }
                    }
                    else
                    {
                        match_read_pos = 0;
                    }
                    search_pos++;
                }
            };

            find_and_replace(search_begin, skip_begin, arg0, arg1);
            size_t write_size = skip_begin - buffer_read_pos;
            push_edit_result(buffer_read_pos, write_size);
            buffer_read_pos += write_size;
            buffer_read_pos += (skip_end - buffer_read_pos);
            find_and_replace(skip_end, search_end, arg0, arg1);
        }

        //Eat strings
        else if (lex->token.type == TokenType_QUOTE)
        {
            while (lex->token.type != TokenType_QUOTE)
            {
                lex_and_require_valid_token(lex);
            }
            lex_and_require_valid_token(lex);
        }
    }

}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("You must provide a filename!\n");
        return -1;
    }

    char *file_buffer;
    size_t file_size;
    {
        FILE* file = fopen(argv[1], "rb");
        if (file == NULL) {
            printf("Could not open file %s\n", argv[1]);
            return 1;
        }
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        file_buffer = (char*)malloc(file_size + 1);
        file_buffer[file_size] = 0;
        fread(file_buffer, 1, file_size, file);
        fclose(file);
    }

	Lexer lex;
    lex.buffer_size = file_size;
    lex.buffer = file_buffer;
    lex.current = file_buffer;
    lex.line_number = 0;
    lex.line_offset = 0;
    lex.is_first_token_in_line = true;
    lex.token.type = TokenType_INVALID;
	
    buffer_read_pos = file_buffer;
    parse_block(&lex, NULL);

    size_t offset = lex.token.text - buffer_read_pos;
    push_edit_result(buffer_read_pos, offset);

    FILE *file = fopen(argv[1], "wb");
    if (file == NULL) return 1;

    EditResult *current_edit = g_root_edit_result;
    while (current_edit != NULL)
    {
        fwrite(current_edit->buffer, 1, current_edit->size, file);
        current_edit = current_edit->next;
    }

    fclose(file);

#if 0
    char input_char = 0;
    printf("Undo Changes(Y/N)?\n");
    scanf("%c", &input_char);
    if (input_char == 'Y' || input_char == 'y') {
        FILE *file = fopen(argv[1], "w");
        if (file == NULL) return 1;
        fwrite(file_buffer, 1, file_size, file);
        fclose(file);
    }
#endif

    //NOTE(Torin) Intentionaly not freeing anything because
    //the operating system is about to do it anyway
    return 0;
}
