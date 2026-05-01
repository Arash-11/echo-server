CC = gcc
CFLAGS = -O0
WFLAGS = -Wall -Werror -Wextra -Wunreachable-code -Wpedantic
COMPILE = $(CC) $(CFLAGS) $(WFLAGS)

TARGET = server
SOURCES = server.c

run: $(TARGET)
	@./$(TARGET)

# r: run # alias, so that I can do `make r` instead of `make run`

$(TARGET): $(SOURCES)
	@$(COMPILE) -o $(TARGET) $(SOURCES)

tcp-echo: tcp-echo.c
	@$(COMPILE) -o $@ $^

clean:
	rm $(TARGET)

.PHONY: run clean
