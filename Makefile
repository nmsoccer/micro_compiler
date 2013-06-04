CC=gcc
C_FLAGS=-c
C_OBJ=-o

MAINOBJS=lexical_analy.o syntax_analy.o semantic_analy.o trans_asm.o compiler.o

target:b

b:$(MAINOBJS)
	$(CC) $(MAINOBJS) $(C_OBJ) $@
	
lexical_analy.o:lexical_analy.c
	$(CC) $(C_FLAGS) $<
syntax_analy.o:syntax_analy.c
	$(CC) $(C_FLAGS) $<
semantic_analy.o:semantic_analy.c
	$(CC) $(C_FLAGS) $<	
trans_asm.o:trans_asm.c
	$(CC) $(C_FLAGS) $<	
compiler.o:compiler.c
	$(CC) $(C_FLAGS) $<
	
clean:
	rm *.o
	rm b			