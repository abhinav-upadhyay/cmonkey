CC=clang
CFLAGS+=-g

all: lexer_tests repl


lexer_tests:	lexer_tests.o lexer.o token.o
	${CC} ${CFLAGS} -o lexer_tests lexer_tests.o lexer.o token.o

repl:	repl.o lexer.o token.o
	${CC} ${CFLAGS} -o repl repl.o lexer.o token.o

lexer_tests.o:	lexer_tests.c
	${CC}  ${CFLAGS} -c lexer_tests.c

repl.o:	repl.c
	${CC} ${CFLAGS} -c repl.c

lexer.o:	lexer.c
	${CC} ${CFLAGS} -c lexer.c

token.o:	token.c
	${CC} ${CFLAGS} -c token.c

clean:
	rm -f lexer_tests repl *.o
