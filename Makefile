NAME = snake
CFLAGS = -std=c99 -Wall -Wextra -Werror -g
LIBFLAGS = -lncurses

OBJS := main.o

all: $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS) $(LIBFLAGS)

clean:
	rm $(NAME) *.o
