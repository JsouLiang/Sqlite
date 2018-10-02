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
	PREPARE_UNRECOGNIZED_STATEMEMT
} PrepareResult;

// Insert , Select .....等命令集合
typedef enum StatementType_t {
	STATEMENT_INSERT,
	STATEMENT_SELECT
} StatementType;

typedef struct Statement_t {
	StatementType type;
} Statement;

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
	// Insert action
	if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
		statement->type = STATEMENT_INSERT;
		return PREPARE_SUCCESS;
	}
	// Select Action
	if (strncmp(input_buffer->buffer, "select", 6) == 0) {
		statement->type = STATEMENT_SELECT;
		return PREPARE_SUCCESS;
	}
	return PREPARE_UNRECOGNIZED_STATEMEMT;
}

void execute_statement(Statement* statement) {
	switch (statement->type) {
		case (STATEMENT_INSERT):
			printf("This is where we would do an insert. \n");
			break;
		case (STATEMENT_SELECT):
			printf("This is where we would do an select. \n");
		default:
			break;
	}
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
	// insert code here...
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
			case (PREPARE_UNRECOGNIZED_STATEMEMT):
				printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
				continue;
			default:
				break;
		}
		execute_statement(&statement);
	}
	return 0;
}
