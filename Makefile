CC=clang
CFLAGS+=-g -D_GNU_SOURCE
SRCDIR := src
OBJDIR := obj
BINDIR := bin

OBJS := $(addprefix $(OBJDIR)/, lexer_tests.o lexer.o token.o repl.o cmonkey_utils.o parser_tests.o)
BINS := $(addprefix $(BINDIR)/, lexer_tests parser_tests repl)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	${COMPILE.c} ${OUTPUT_OPTION}  $<

all: $(OBJS) $(BINS) lexer_tests parser_tests repl

$(OBJS): | $(OBJDIR)

$(BINS): $(OBJS) | ${BINDIR}

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

lexer_tests:	${OBJDIR}/lexer_tests.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o
	${CC} ${CFLAGS} -o ${BINDIR}/lexer_tests ${OBJDIR}/lexer_tests.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o

parser_tests:	${OBJDIR}/parser_tests.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o $(OBJDIR)/parser.o $(OBJDIR)/cmonkey_utils.o
	${CC} ${CFLAGS} -o ${BINDIR}/parser_tests ${OBJDIR}/parser_tests.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o $(OBJDIR)/parser.o $(OBJDIR)/cmonkey_utils.o

repl:	${OBJDIR}/repl.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o
	${CC} ${CFLAGS} -o ${BINDIR}/repl ${OBJDIR}/repl.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o

clean:
	rm -rf $(BINDIR) $(OBJDIR) core
