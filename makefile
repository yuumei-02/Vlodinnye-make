.ONESHELL:
.PHONY: example

compiler := gcc
cflags := -Wall -Wextra -pedantic -std=c23 -Og -ggdb
clibs := -lmcu-debug

example:
	$(compiler) $(cflags) *.c -o vmake $(clibs)

