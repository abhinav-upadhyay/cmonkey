all: lexer_tests


lexer_tests:	lexer_tests.o lexer.o
	${CC} ${CFLAGS} -o lexer_tests lexer_tests.o lexer.o

lexer_tests.o:	lexer_tests.c
	${CC}  ${CFLAGS} -c lexer_tests.c

lexer.o:	lexer.c
	${CC} ${CFLAGS} -c lexer.c

clean:
	rm -f lexer_tests *.o
