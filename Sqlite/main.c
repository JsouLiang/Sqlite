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

int main(int argc, const char * argv[]) {
	// insert code here...
	InputBuffer* input_buffer = new_input_buffer();
	while (true) {
		print_prompt();
		read_input(input_buffer);
		
		if (strcmp(input_buffer->buffer, ".exit") == 0) {
			exit(EXIT_FAILURE);
		} else {
			printf("Unrecognized command '%s'.\n", input_buffer->buffer);
		}
	}
	return 0;
}
