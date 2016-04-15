#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <malloc.h>
#include <iostream>

#include <vector>

enum DependencyType {
	DependencyType_FUNCTION,
	DependencyType_STRUCTURE,
	DependencyType_GLOBAL,
};

struct Code_Dependency {
	const char* identifier;
	DependencyType type;
};

struct Variable_Declaration {
	const char* name;
	size_t name_length;
	const char* type_name;
	size_t type_name_length;
	uint32_t indirection_level;
	uint32_t qualifiers;
};

struct Function_Definition {
	const char* name;
	size_t name_length;

	Variable_Declaration* argument_decls;
	size_t argument_count;

	Code_Dependency* dependices;
	size_t dependency_count;

	const char* function_text;
};

struct Memory_Block {
	uint8_t* data;
	size_t size;
	size_t used;
};

void create_memory_block(Memory_Block* block, size_t size) {
	block->data = (uint8_t*)malloc(size);
	block->size = size;
}

void free_memory_block(Memory_Block* block) {
	free(block);
}

struct Linked_Memory_Block {
	uint8_t* data;
	size_t size;
	Linked_Memory_Block* previous;
};

struct Block_Allocator 
{
	Linked_Memory_Block* current_block;
	size_t memory_used;
	size_t block_size;
};

static void init_block_allocator(Block_Allocator* allocator, size_t inital_size) 
{
	Linked_Memory_Block* first_block = (Linked_Memory_Block*)malloc(sizeof(Linked_Memory_Block) + inital_size);
	first_block->data = (uint8_t*)first_block + 1;
	allocator->memory_used = 0;
	allocator->current_block = first_block;
}

static void free_memory_block(Linked_Memory_Block* block) {
	if (block->previous != nullptr) {
		free_memory_block(block->previous);
	}

	free(block->data);
}

uint8_t* allocate(Block_Allocator* allocator, size_t size) {
	uint8_t* result;
	if (allocator->memory_used + size < allocator->current_block->size) {
		result = &allocator->current_block->data[allocator->memory_used];
		allocator->memory_used += size;
		return result;
	} else {
		Linked_Memory_Block* block = (Linked_Memory_Block*)malloc(sizeof(Linked_Memory_Block) + allocator->block_size);
		block->previous = allocator->current_block;
		block->size = allocator->block_size;
		block->data = (uint8_t*)(block + 1);
		allocator->current_block = block;
		return allocate(allocator, size);
	}
}

static void free_block_allocator(Block_Allocator* allocator)
{

}

char* read_file(const char* filename) {
	FILE* file = fopen(filename, "r");
	if (file) {	
		fseek(file, 0, SEEK_END);
		long file_size = ftell(file);
		char* buffer = (char*)malloc(file_size + 1);
		fseek(file, 0, SEEK_SET);
		fread(buffer, 1, file_size, file);
		buffer[file_size] = 0;
		fclose(file);
		return buffer;
	}
	return 0;	
}

enum TokenType {
	TokenType_UNKNOWN,	
	TokenType_IDENTIFIER, 
	TokenType_PAREN_OPEN 	= '(',
	TokenType_PAREN_CLOSE	= ')',
	TokenType_BRACE_OPEN 	= '{',
	TokenType_BRACE_CLOSE 	= '}',
	TokenType_STRUCT
};

struct Token {
	TokenType type;
	char* text;
	size_t text_length;
};

struct Tokenizer {
	char* current;
	Token token;
};

inline bool is_alpha(char c) {
	bool result = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
	return result;	
}

inline bool is_number(char c) {
	bool result = (c >= '0' && c <= '9');
	return result;
}

inline void eat_whitespace(Tokenizer* tokenizer) {
	while (*tokenizer->current == '\n' ||
			*tokenizer->current == '\t' ||
			*tokenizer->current == ' ') {
		tokenizer->current++;
	}
}

inline void eat_statement(Tokenizer* tokenizer) {
	while(tokenizer->current[0] != ';' && tokenizer->current[0] != 0)
		tokenizer->current++;
}

static inline bool string_equal(const char* string_a, size_t length_a, const char* string_b, size_t length_b)
{
	if (length_a != length_b) return 0;
	for (size_t i = 0; i < length_a; i++)
	{
		if (string_a[i] != string_b[i]) return 0;
	}
	return 1;
}

void next_token(Tokenizer* tokenizer) {
	Token result;
	eat_whitespace(tokenizer);
	if (is_alpha(*tokenizer->current) || *tokenizer->current == '_') {
		result.text = tokenizer->current;
		while(*tokenizer->current != 0 && 
				(is_alpha(*tokenizer->current) 
				|| is_number(*tokenizer->current) 
				|| *tokenizer->current == '_')) {	
			tokenizer->current++;	
		}

		result.text_length = tokenizer->current - result.text;
		if (string_equal(result.text, result.text_length, "struct", sizeof("struct"))) {
			result.type = TokenType_STRUCT;
		} else {
			result.type = TokenType_IDENTIFIER;
		}
		
	} else {
		result.type = (TokenType)*tokenizer->current;
		result.text = tokenizer->current;
		tokenizer->current++;
		result.text_length = 1;
	}

	tokenizer->token = result;
}

#if 0
static inline void parse_struct_defn(Tokenizer* tokenizer) {
	Token ident_token = get_token(tokenizer);
	if (ident_token.type == TokenType_IDENTIFIER) {
		Token token = get_token(tokenizer);
		if (token.type == TokenType_BRACE_OPEN) {
			int brace_level = 1;
			while(brace_level > 0) {
				token = get_token(tokenizer);
				if (token.type == TokenType_BRACE_OPEN) {
					brace_level++;
				} else if (token.type == TokenType_BRACE_CLOSE) {
					brace_level--;
				}
			}

			printf("parsed struct defn\n");
			fwrite(ident_token.text, 1, ident_token.text_length, stdout);
			printf("\n");
		}	
	}	
}
#endif

enum Qualifier {
	Qualifer_STATIC = 1 << 0,
	Qualifer_EXTERN = 1 << 1,
	Qualifer_INLINE = 1 << 2,
	Qualifer_CONST  = 1 << 3,
};

static inline int get_qualifer(Token* token) {
	if (string_equal(token->text, token->text_length, "static", 6)) {
		return Qualifer_STATIC;
	} else if (string_equal(token->text, token->text_length, "extern", 6)) {
		return Qualifer_EXTERN;
	} else if (string_equal(token->text, token->text_length, "inline", 6)) {
		return Qualifer_INLINE;
	} else if (string_equal(token->text, token->text_length, "const", 5)) {
		return Qualifer_CONST;
	}

	return 0;
}

static uint32_t parse_qualifers(Tokenizer* tokenizer) {
	uint32_t result = 0;
	int last_qualifer = get_qualifer(&tokenizer->token);
	while (last_qualifer != 0) {
		result |= last_qualifer;
		next_token(tokenizer);
		last_qualifer = get_qualifer(&tokenizer->token);
	}
	return result;
}	

struct Identifier {
	const char* name;
	size_t length;
};

static std::vector<Identifier> dependencies;
inline void add_dependency(Token token) {
	for (auto& ident : dependencies) {
		if (string_equal(ident.name, ident.length, token.text, token.text_length)) {
			return;
		}
	}	

	Identifier ident;
	ident.name = token.text;
	ident.length = token.text_length;
	dependencies.emplace_back(ident);
}

inline bool is_pointer_or_refrence(Token* token) {
	bool result = token->text[0] == '*' || token->text[0] == '&';
	return result;
}

void print(Variable_Declaration* decl) {
	fwrite(decl->type_name, 1, decl->type_name_length, stdout);
	printf(" ");
	fwrite(decl->name, 1, decl->name_length, stdout);
	printf("\n");
}

void print(Function_Definition* function) {
	fwrite(function->name, 1, function->name_length, stdout);
	printf("(");
	for (size_t i = 0; i < function->argument_count; i++) {
		fwrite(function->argument_decls[i].name, 1, function->argument_decls[i].name_length, stdout);
		printf(" ");
		fwrite(function->argument_decls[i].type_name, 1, function->argument_decls[i].type_name_length, stdout);
		printf(",");
	}
	printf("\n");
	printf("Depends On: \n");
}

void parse_file(const char* filename) {
	Tokenizer tokenizer;
	char* buffer = read_file(filename);
	if (buffer == 0) {	
		printf("Could not read file: %s", filename);
		return;
	}

	tokenizer.current = buffer;
	while (*tokenizer.current != 0) {
		eat_whitespace(&tokenizer);
		next_token(&tokenizer);
		if (tokenizer.token.type == TokenType_STRUCT) {
		} else {
			uint32_t qualifers = parse_qualifers(&tokenizer);
			if (tokenizer.token.type == TokenType_IDENTIFIER) {
				Token type_ident = tokenizer.token;
				next_token(&tokenizer);
				if (tokenizer.token.type == TokenType_IDENTIFIER) {
					Token decl_ident = tokenizer.token;
					next_token(&tokenizer);
					if (tokenizer.token.type == TokenType_PAREN_OPEN) {
						next_token(&tokenizer);

						
						std::vector<Variable_Declaration> declerations;
						while (tokenizer.token.type != TokenType_PAREN_CLOSE) {
							declerations.emplace_back();
							Variable_Declaration* decl = &declerations.back();
							decl->qualifiers = parse_qualifers(&tokenizer);
							decl->type_name = tokenizer.token.text;
							decl->type_name_length = tokenizer.token.text_length;
							next_token(&tokenizer);	
							while (is_pointer_or_refrence(&tokenizer.token)) {
								next_token(&tokenizer);
								decl->indirection_level++;
							}

							decl->name = tokenizer.token.text;
							decl->name_length = tokenizer.token.text_length;
							next_token(&tokenizer);
						}


					}
					add_dependency(decl_ident);
				}
			}	
		}
	}	

	for (auto& ident : dependencies) {
		printf("dependency: ");
		fwrite(ident.name, 1, ident.length, stdout);
		printf("\n");		
	}

	free(buffer);
}
