CC=clang
CFLAGS+=-g -D_GNU_SOURCE -D_OPENBSD_SOURCE -Wall --std=c11
SRCDIR := src
OBJDIR := obj
BINDIR := bin

OBJS := $(addprefix $(OBJDIR)/, lexer_tests.o lexer.o token.o repl.o \
	cmonkey_utils.o parser_tracing.o parser_tests.o evaluator_tests.o object.o \
	cmonkey_utils_tests.o environment.o builtins.o object_tests.o opcode.o \
	opcode_tests.o compiler_tests.o test_utils.o compiler_tests.o compiler.o \
	vm.o vm_tests.o)
BINS := $(addprefix $(BINDIR)/, lexer_tests parser_tests evaluator_tests \
	cmonkey_utils_tests object_tests opcode_tests compiler_tests vm_tests \
	monkey)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	${COMPILE.c} ${OUTPUT_OPTION}  $<

all: $(OBJS) $(BINS) lexer_tests parser_tests evaluator_tests cmonkey_utils_tests \
	object_tests opcode_tests compiler_tests vm_tests monkey

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
	$(OBJDIR)/environment.o $(OBJDIR)/builtins.o $(OBJDIR)/test_utils.o
	${CC} ${CFLAGS} -o ${BINDIR}/evaluator_tests ${OBJDIR}/evaluator_tests.o ${OBJDIR}/lexer.o \
		${OBJDIR}/token.o $(OBJDIR)/parser.o $(OBJDIR)/cmonkey_utils.o $(OBJDIR)/parser_tracing.o \
		$(OBJDIR)/evaluator.o $(OBJDIR)/object.o $(OBJDIR)/environment.o $(OBJDIR)/builtins.o $(OBJDIR)/test_utils.o

cmonkey_utils_tests: $(OBJDIR)/cmonkey_utils.o $(OBJDIR)/cmonkey_utils_tests.o
	$(CC) $(CFLAGS) -o $(BINDIR)/cmonkey_utils_tests $(OBJDIR)/cmonkey_utils.o $(OBJDIR)/cmonkey_utils_tests.o

object_tests: $(OBJDIR)/cmonkey_utils.o $(OBJDIR)/object_tests.o $(OBJDIR)/object.o \
	$(OBJDIR)/parser.o $(OBJDIR)/token.o $(OBJDIR)/lexer.o
	$(CC) $(CFLAGS) -o $(BINDIR)/object_tests $(OBJDIR)/cmonkey_utils.o $(OBJDIR)/object_tests.o \
	$(OBJDIR)/object.o $(OBJDIR)/parser.o $(OBJDIR)/token.o $(OBJDIR)/lexer.o

opcode_tests: $(OBJDIR)/opcode_tests.o $(OBJDIR)/opcode.o $(OBJDIR)/cmonkey_utils.o
	$(CC) $(CFLAGS) -o $(BINDIR)/opcode_tests $(OBJDIR)/opcode_tests.o $(OBJDIR)/opcode.o $(OBJDIR)/cmonkey_utils.o

compiler_tests: $(OBJDIR)/compiler_tests.o $(OBJDIR)/compiler.o $(OBJDIR)/cmonkey_utils.o $(OBJDIR)/test_utils.o \
	$(OBJDIR)/object.o $(OBJDIR)/parser.o $(OBJDIR)/token.o $(OBJDIR)/lexer.o $(OBJDIR)/opcode.o
	$(CC) $(CFLAGS) -o $(BINDIR)/compiler_tests $(OBJDIR)/compiler_tests.o $(OBJDIR)/compiler.o \
		$(OBJDIR)/cmonkey_utils.o $(OBJDIR)/test_utils.o $(OBJDIR)/object.o $(OBJDIR)/parser.o $(OBJDIR)/token.o \
		$(OBJDIR)/lexer.o $(OBJDIR)/opcode.o

vm_tests: $(OBJDIR)/vm_tests.o $(OBJDIR)/compiler.o $(OBJDIR)/test_utils.o \
	$(OBJDIR)/parser.o $(OBJDIR)/lexer.o $(OBJDIR)/token.o ${OBJDIR}/object.o \
	$(OBJDIR)/cmonkey_utils.o $(OBJDIR)/opcode.o $(OBJDIR)/vm.o
	$(CC) $(CFLAGS) -o $(BINDIR)/vm_tests $(OBJDIR)/vm_tests.o $(OBJDIR)/compiler.o \
		$(OBJDIR)/test_utils.o $(OBJDIR)/parser.o $(OBJDIR)/lexer.o $(OBJDIR)/token.o \
		$(OBJDIR)/object.o $(OBJDIR)/cmonkey_utils.o $(OBJDIR)/opcode.o $(OBJDIR)/vm.o

monkey:	${OBJDIR}/repl.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o $(OBJDIR)/parser.o $(OBJDIR)/cmonkey_utils.o \
	$(OBJDIR)/evaluator.o ${OBJDIR}/object.o $(OBJDIR)/environment.o $(OBJDIR)/builtins.o
	${CC} ${CFLAGS} -o ${BINDIR}/monkey ${OBJDIR}/repl.o ${OBJDIR}/lexer.o ${OBJDIR}/token.o $(OBJDIR)/parser.o \
		$(OBJDIR)/cmonkey_utils.o ${OBJDIR}/evaluator.o $(OBJDIR)/object.o $(OBJDIR)/environment.o $(OBJDIR)/builtins.o

clean:
	rm -rf $(BINDIR) $(OBJDIR) core
