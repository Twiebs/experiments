#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdint.h>

#include <vector>
#include <functional>

struct Identifier {
	const char* name;
};

struct Function_Definition {
	const char* name;
	const char* return_type_name;
	const char* body;
};

uint64_t make_string_uuid(const char* string) {
	uint64_t result = 5381;
	const char* c = string;
	while (*c) {
		result = ((result << 5) + result) + *c;
		c++;
	}

	return result;
}

struct Library {
	const char* name;
	size_t function_count;
	Function_Definition* functions;
};

#define write_string(string, file) fwrite(string, 1, sizeof(string), file)

static inline char uppercase(char c) {
	if (c >= 'a' && c <= 'z') c += ('A' - 'a');
	return c;
}

static inline void write_uppercase(const char* string, char* buffer) {
	size_t length = strlen(string);
	for (size_t i = 0; i < length; i++) {
		buffer[i] = uppercase(string[i]);
	}
}

static inline void write_generation_notice(FILE* file) {
	static const char *generation_notice =
		"//This file was generated using the kode_depot tool for library creation\n"
		"//Here is another random line of text that is useless\n\n";
	fprintf(file, generation_notice);
}

static inline void write_function_definition(FILE* file, Function_Definition* function) {
	fprintf(file, "void %s() {\n", function->name);
	fprintf(file, "}\n\n");
}

static inline void write_function_decleration(FILE* file, Function_Definition* function) {
	fprintf(file, "void %s();\n", function->name);
}

static void write_library_file(Library* library, const char* filename) {
	FILE* file = fopen(filename, "w");
	if (!file) return;

	write_generation_notice(file);
	char buffer[1024];
	memset(buffer, 0, 1024);
for (size_t i = 0; i < library->function_count; i++) {
	write_function_decleration(file, &library->functions[i]);
}

fprintf(file, "\n\n");
write_uppercase(library->name, buffer);
fprintf(file, "#ifndef %s_IMPLEMENTATION\n", buffer);
fprintf(file, "#define %s_IMPLEMENTATION\n", buffer);
for (size_t i = 0; i < library->function_count; i++) {
	write_function_definition(file, &library->functions[i]);
}

fprintf(file, "#endif//%s_IMPLEMENTATION", buffer);
fclose(file);
}


void debug_init_test_library(Library* library) {
	library->function_count = 2;
	library->functions = (Function_Definition*)malloc(sizeof(Function_Definition) * library->function_count);
	library->functions[0] = { "bollocks" };
	library->functions[1] = { "giant_cactus" };
}

struct GetFilesResult {
	char filename[256];	
	uint64_t last_write_time;
};

#include <Windows.h>
int get_files_in_directory(const char* directory, char* filename_buffer, char* directory_buffer, uint32_t* file_count, uint32_t* directory_count) {
	WIN32_FIND_DATA find_data;
	HANDLE handle = FindFirstFile(directory, &find_data);
	if (handle == INVALID_HANDLE_VALUE) {
		printf("Win32ERROR: %", GetLastError());
		return 1;
	}

	size_t filename_write_pos = 0;
	size_t directory_write_pos = 0;
	do {
		if (strcmp(find_data.cFileName, ".") == 0) continue;
		else if (strcmp(find_data.cFileName, "..") == 0) continue;

		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			size_t length = strlen(find_data.cFileName);
			memcpy(&directory_buffer[directory_write_pos], find_data.cFileName, length);
			directory_buffer[directory_write_pos + length] = 0;
			directory_write_pos += length + 1;
			*directory_count += 1;
		} else {
			size_t length = strlen(find_data.cFileName);
			memcpy(&filename_buffer[filename_write_pos], find_data.cFileName, length);
			filename_buffer[filename_write_pos + length] = 0;
			filename_write_pos += length + 1;
			*file_count += 1;
		}

	} while (FindNextFile(handle, &find_data));

	return 0;
}

#define KILOBYTES(x) ((x) << 10)
#define MEGABYTES(x) ((x) << 20)
#define GIGABYTES(x) ((x) << 30)

#define DATABASE_FILE_MAGIC_NUMBER ((1 << 6) + 20)
#define PROJECT_FILE_MAGIC_NUMBER ((1 << 10) + 346530)

struct Database_File_Header {
	uint64_t magic_number;
};

struct Database_Entry_Data {
	uint64_t last_write_time;
};

struct Database {
	const char* repository_path;
	size_t repository_path_length;

	uint32_t imported_file_count;
	uint32_t imported_uuid_count;

	uint64_t* imported_file_hashes;
	uint32_t* imported_uuid_per_file;
	uint64_t* imported_uuids;
};

enum Modifcation_Type {
	Modification_Type_NEW_FILE,
};

struct Modification {
	Modifcation_Type type;
	uint64_t file_hash;
	int64_t delta_uuids;
};

//NOTE(Torin) vectors are used to prototype what data
//The database needs to care about when somthing has changed
struct Database_Modifications {
	std::vector<uint64_t> added_hashes;
	std::vector<uint32_t> added_uuid_count_per_hash;

	size_t added_uuid_memory_capacity;
	size_t added_uuid_used_memory;
	uint64_t* added_uuids;
};

void parse_file(const char* filename);


void array_create(size_t inital_capacity, size_t size, void** buffer, size_t* used, size_t* capacity) {
	*buffer = malloc(size * inital_capacity);
	*used = 0;
	*capacity = inital_capacity;
}

void* array_add(size_t size, void** buffer, size_t* used, size_t* capacity) {
	if (*used + size > *capacity) {
		size_t new_capacity = *capacity + 10;
		void* new_buffer = malloc(size * new_capacity);
		memcpy(new_buffer, *buffer, size * (*capacity));
		free(*buffer);
		*buffer = new_buffer;
		*capacity = new_capacity;
	}

	void* result = (void*)((size_t)*buffer + *used);
	*used += size;
	return result;
}

static inline bool contains_file_hash(Database* database, uint64_t file_hash) {
	for (uint32_t i = 0; i < database->imported_file_count; i++) {
		if (database->imported_file_hashes[i] == file_hash) 
			return true;
	}
	return false;
}


static void write_database_to_file(Database* database, Database_Modifications* modifications, const char* filename) {
	FILE* file = fopen(filename, "w");
	if (file == nullptr) return;

	Database_File_Header header;
	header.magic_number = DATABASE_FILE_MAGIC_NUMBER;
	fwrite(&header.magic_number, sizeof(uint64_t), 1, file);

	uint64_t hash_count = modifications->added_hashes.size() + database->imported_file_count;
	fwrite(&hash_count, sizeof(uint64_t), 1, file);
	fwrite(&database->imported_file_hashes, sizeof(uint64_t), database->imported_file_count, file);
	fwrite(modifications->added_hashes.data(), sizeof(uint64_t), modifications->added_hashes.size(), file);
	fclose(file);
}

int read_database_from_file(Database* database, const char* filename) {
	FILE* file = fopen(filename, "r");
	if (file == nullptr) return 0;

	Database_File_Header header;
	fread(&header.magic_number, sizeof(uint64_t), 1, file);
	if (header.magic_number != DATABASE_FILE_MAGIC_NUMBER) {
		fclose(file);
		return 0;
	}

	fread(&database->imported_file_count, sizeof(uint64_t), 1, file);
	database->imported_file_hashes = (uint64_t*)malloc(sizeof(uint64_t) * database->imported_file_count);
	fread(database->imported_file_hashes, sizeof(uint64_t), database->imported_file_count, file);
	fclose(file);	
	return 1;
}

static inline 
void update_file(const char* filename, uint64_t last_write_time,
	Database* database, Database_Modifications* modifications) 
{
	uint64_t file_hash = make_string_uuid(filename);
	if (!contains_file_hash(database, file_hash)) {
		modifications->added_hashes.emplace_back(file_hash);
	} else {

	}



}


#include <direct.h>
int main(int argc, char** argv) {
	Library library;
	debug_init_test_library(&library);
	library.name = "test";

	_mkdir(".internal");

	Database database;
	database.repository_path = "../repo";
	database.repository_path_length = sizeof("../repo");	
	if (!read_database_from_file(&database, ".internal/database.kdb")) {
		database.imported_file_count = 0;
		database.imported_file_hashes = nullptr;	
	}

	//NOTE(Torin) The memory created for the database modifications is not freed on exit
	//because it is persistant throughout the lifetime of the application
	Database_Modifications modifications;
	array_create(4096, sizeof(uint64_t), (void**)&modifications.added_uuids, 
		&modifications.added_uuid_used_memory, &modifications.added_uuid_memory_capacity);


	char filenames[KILOBYTES(10)];
	char directories[KILOBYTES(10)];
	uint32_t file_count = 0, directory_count = 0;
	get_files_in_directory("../repo/*", filenames, directories, &file_count, &directory_count);

	size_t read_pos = 0;
	char string_buffer[4096];
	memcpy(string_buffer, SEARCH_DIR, sizeof(SEARCH_DIR));
	for (uint32_t i = 0; i < file_count; i++) {
		uint64_t file_hash = make_string_uuid(&filenames[read_pos]);
		if (!contains_file_hash(&database, file_hash)) {
			modifications.added_hashes.emplace_back(make_string_uuid(&filenames[read_pos]));
		} else {

		}

		size_t length = strlen(&filenames[read_pos]);
		memcpy(string_buffer + sizeof(SEARCH_DIR) - 1, &filenames[read_pos], length);
		string_buffer[sizeof(SEARCH_DIR) - 1 + length] = 0;
		read_pos += length + 1;
		parse_file(string_buffer);
	}

	write_database_to_file(&database, &modifications, ".internal/database.kdb");

	return 0;
}