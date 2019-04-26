CC=clang
CFLAGS+=-g -D_GNU_SOURCE -Wall
SRCDIR := src
OBJDIR := obj
BINDIR := bin

OBJS := $(addprefix $(OBJDIR)/, lexer_tests.o lexer.o token.o repl.o cmonkey_utils.o parser_tracing.o \
	parser_tests.o evaluator_tests.o object.o cmonkey_utils_tests.o environment.o builtins.o object_tests.o)
BINS := $(addprefix $(BINDIR)/, lexer_tests parser_tests evaluator_tests cmonkey_utils_tests object_tests repl)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	${COMPILE.c} ${OUTPUT_OPTION}  $<

all: $(OBJS) $(BINS) lexer_tests parser_tests evaluator_tests cmonkey_utils_tests object_tests repl

$(OBJS): | $(OBJDIR)

$(BINS): $(OBJS) | ${BINDIR}

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

lexer_tests:	${OBJDIR}/lexer_tests.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o
	${CC} ${CFLAGS} -o ${BINDIR}/lexer_tests ${OBJDIR}/lexer_tests.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o

parser_tests:	${OBJDIR}/parser_tests.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o $(OBJDIR)/parser.o \
	$(OBJDIR)/cmonkey_utils.o $(OBJDIR)/parser_tracing.o
	${CC} ${CFLAGS} -o ${BINDIR}/parser_tests ${OBJDIR}/parser_tests.o ${OBJDIR}/lexer.o \
		${OBJDIR}/token.o $(OBJDIR)/parser.o $(OBJDIR)/cmonkey_utils.o $(OBJDIR)/parser_tracing.o

evaluator_tests:	${OBJDIR}/evaluator.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o $(OBJDIR)/parser.o \
	$(OBJDIR)/cmonkey_utils.o $(OBJDIR)/parser_tracing.o $(OBJDIR)/evaluator.o $(OBJDIR)/object.o \
	$(OBJDIR)/environment.o $(OBJDIR)/builtins.o
	${CC} ${CFLAGS} -o ${BINDIR}/evaluator_tests ${OBJDIR}/evaluator_tests.o ${OBJDIR}/lexer.o \
		${OBJDIR}/token.o $(OBJDIR)/parser.o $(OBJDIR)/cmonkey_utils.o $(OBJDIR)/parser_tracing.o \
		$(OBJDIR)/evaluator.o $(OBJDIR)/object.o $(OBJDIR)/environment.o $(OBJDIR)/builtins.o

cmonkey_utils_tests: $(OBJDIR)/cmonkey_utils.o $(OBJDIR)/cmonkey_utils_tests.o
	$(CC) $(CFLAGS) -o $(BINDIR)/cmonkey_utils_tests $(OBJDIR)/cmonkey_utils.o $(OBJDIR)/cmonkey_utils_tests.o

object_tests: $(OBJDIR)/cmonkey_utils.o $(OBJDIR)/object_tests.o $(OBJDIR)/object.o \
	$(OBJDIR)/parser.o $(OBJDIR)/token.o $(OBJDIR)/parser.o $(OBJDIR)/lexer.o 
	$(CC) $(CFLAGS) -o $(BINDIR)/object_tests $(OBJDIR)/cmonkey_utils.o $(OBJDIR)/object_tests.o \
	$(OBJDIR)/object.o $(OBJDIR)/parser.o $(OBJDIR)/token.o $(OBJDIR)/lexer.o


repl:	${OBJDIR}/repl.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o $(OBJDIR)/parser.o $(OBJDIR)/cmonkey_utils.o \
	$(OBJDIR)/evaluator.o ${OBJDIR}/object.o $(OBJDIR)/environment.o $(OBJDIR)/builtins.o
	${CC} ${CFLAGS} -o ${BINDIR}/repl ${OBJDIR}/repl.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o $(OBJDIR)/parser.o \
		$(OBJDIR)/cmonkey_utils.o ${OBJDIR}/evaluator.o $(OBJDIR)/object.o $(OBJDIR)/environment.o $(OBJDIR)/builtins.o

clean:
	rm -rf $(BINDIR) $(OBJDIR) core
