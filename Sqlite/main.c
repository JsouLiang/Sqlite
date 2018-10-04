//
//  main.c
//  Sqlite
//
//  Created by Liang on 2018/9/28.
//  Copyright © 2018 imieutan. All rights reserved.
//

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct InputButter_t {
	char* buffer;
	size_t buffer_length;
	size_t input_length;
} InputBuffer;

InputBuffer* new_input_buffer() {
	InputBuffer* input_buffer = malloc(sizeof(InputBuffer));
	input_buffer->buffer = NULL;
	input_buffer->buffer_length = 0;
	input_buffer->input_length = 0;
	return input_buffer;
}

/**
 打印提示
 */
void print_prompt() { printf("db > "); }

void read_input(InputBuffer* input_buffer) {
	ssize_t byte_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);
	if (byte_read <= 0) {
		printf("Error reading input\n");
		exit(EXIT_FAILURE);
	}
	
	// input_length 是输入内容删除掉最后的 \n 的长度
	input_buffer->input_length = byte_read - 1;
	// 不要忘记char* 字符串以0 结尾
	input_buffer->buffer[byte_read - 1] = 0;
}

typedef enum MetaCommandResult_t {
	META_COMMAND_SUCCESS,
	META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum PrepareResult_t {
	PREPARE_SUCCESS,
	PREPARE_UNRECOGNIZED_STATEMEMT,
	PREPARE_SYNTAX_ERROR	// Syntax error 语法错误
} PrepareResult;

/*
 simulate simple table insert action
 TABLE:
 id				integer
 username		varchar(32)
 emial			varchar(255)
 */
const uint32_t COLUMN_USERNAME_SIZE = 32;
const uint32_t COLUMN_EMIAL_SIZE = 255;
typedef struct Row_t {
	uint32_t id;
	char username[COLUMN_USERNAME_SIZE];
	char email[COLUMN_EMIAL_SIZE];
} Row;

// Insert , Select .....等指令集合
typedef enum StatementType_t {
	STATEMENT_INSERT,
	STATEMENT_SELECT
} StatementType;

typedef struct Statement_t {
	StatementType type;
	Row row_to_insert;			// only used by insert statement
} Statement;

/**
 				size			offset
 id				4				0
 username		32				4
 email			255				36
 total			291
 */
#define SIZE_OF_ATTRIBUTE(Struct, Attribute) sizeof(((Struct*)0)->Attribute)
const uint32_t ID_SIZE = SIZE_OF_ATTRIBUTE(Row, id);
const uint32_t USERNAME_SIZE = SIZE_OF_ATTRIBUTE(Row, username);
const uint32_t EMAIL_SIZE = SIZE_OF_ATTRIBUTE(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

/**
 序列化，将Row 数据从source中拷贝到destination地址上去
 */
void serialize_row(Row* source, void* destination) {
	memcmp(destination + ID_OFFSET, &(source->id), ID_SIZE);
	memcmp(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
	memcmp(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void* source, Row* destionation) {
	memcmp(&(destionation->id), source + ID_OFFSET, ID_SIZE);
	memcmp(&(destionation->username), source + USERNAME_OFFSET, USERNAME_SIZE);
	memcmp(&(destionation->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

// TABLE
const uint32_t PAGE_SIZE = 1024 * 4;								// 每一页大小
const uint32_t TABLE_MAX_PAGE = 100;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;				// 每一页行数
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGE;
// Table struct that points to pages of rows and keeps track of how many rows there are
typedef struct Table_t {
	void* pages[TABLE_MAX_PAGE];
	uint32_t num_rows;
} Table;

typedef enum ExecuteResult_t {
	EXECUTE_SUCCESS,
	EXECUTE_TABLE_FULL
} ExecuteResult;

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
	// Insert action
	// strncmp 对比insert->buffer 和insert中前6个字符
	if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
		statement->type = STATEMENT_INSERT;
		// insert 1 cstack foo@bar.com  ; id, name, email
		int args_assigned = sscanf(
			input_buffer->buffer,
								   "insert %d %s %s", &(statement->row_to_insert.id), statement->row_to_insert.username, statement->row_to_insert.email);
		if (args_assigned < 3) {
			return PREPARE_SYNTAX_ERROR;
		}
		
		return PREPARE_SUCCESS;
	}
	// Select Action
	if (strncmp(input_buffer->buffer, "select", 6) == 0) {
		statement->type = STATEMENT_SELECT;
		return PREPARE_SUCCESS;
	}
	return PREPARE_UNRECOGNIZED_STATEMEMT;
}

void* row_slot(Table* table, uint32_t row_num) {
	// 计算要插入的行在第几页
	uint32_t page_num = row_num / ROWS_PER_PAGE;
	void* page = table->pages[page_num];
	if (!page) {
		page = table->pages[page_num] = malloc(PAGE_SIZE);
	}
	// 偏移到第几行
	uint32_t row_offset = row_num % ROWS_PER_PAGE;
	// 偏移到第几行的字节大小
	uint32_t byte_offset = row_offset * ROW_SIZE;
	return page + byte_offset;
}


ExecuteResult execute_insert(Statement* statement, Table* table) {
	if (table->num_rows >= TABLE_MAX_ROWS) {
		return EXECUTE_TABLE_FULL;
	}
	Row* row_to_insert = &(statement->row_to_insert);
	serialize_row(row_to_insert, row_slot(table, table->num_rows));
	table->num_rows += 1;
	return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* statement, Table* table) {
	Row row;
	for (uint32_t i = 0; i < table->num_rows; i++) {
		deserialize_row(row_slot(table, i), &row);
		
	}
	return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement* statement, Table* table) {
	switch (statement->type) {
		case (STATEMENT_INSERT):
			return execute_insert(statement, table);
		case (STATEMENT_SELECT):
			return execute_select(statement, table);
	}
}

Table* new_table() {
	Table* table = malloc(sizeof(Table));
	table->num_rows = 0;
	return table;
}

// 解析以 . dot 开头的指令
// .exit 退出
MetaCommandResult do_meta_command(InputBuffer* input_buffer) {
	if (strcmp(input_buffer->buffer, ".exit") == 0) {
		exit(EXIT_SUCCESS);
	} else {
		return META_COMMAND_UNRECOGNIZED_COMMAND;
	}
}

int main(int argc, const char * argv[]) {
	Table* table = new_table();
	InputBuffer* input_buffer = new_input_buffer();
	while (true) {
		print_prompt();
		read_input(input_buffer);
		// 声明(Statement) 以 . 开头(start with a dot)
		if (input_buffer->buffer[0] == '.') {
			switch (do_meta_command(input_buffer)) {
				case (META_COMMAND_SUCCESS):
					continue ;
				case (META_COMMAND_UNRECOGNIZED_COMMAND):
					printf("Unrecoginzed command '%s'\n", input_buffer->buffer);
					continue;
			}
		}
		
		// 解析操作指令
		Statement statement;
		switch (prepare_statement(input_buffer, &statement)) {
			case (PREPARE_SUCCESS):
				break;
			case PREPARE_SYNTAX_ERROR:
				printf("Syntax error");
				continue;
			case (PREPARE_UNRECOGNIZED_STATEMEMT):
				printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
				continue;
			default:
				break;
		}
		
		switch (execute_statement(&statement, table)) {
			case EXECUTE_SUCCESS:
				printf("Executed.");
				break;
			case EXECUTE_TABLE_FULL:
				printf("Error: Table full\n");
			default:
				break;
		}
	}
	return 0;
}
