all: worms

worms: worms.c

CFLAGS+= -W -Wall -Werror -Wextra
