CC =  gcc # Set the compiler
L_FLAGS = -lrt -lpthread -lm
#C_FLAGS = -g

all: pt_a1 pt_a2 pt_a3
.PHONY: all

# Project compilation
pt_a1: A1/periodicTask.c
	$(CC) $< -o $@ $(C_FLAGS) $(L_FLAGS)

pt_a2: A2/periodicTask.c
	$(CC) $< -o $@ $(C_FLAGS) $(L_FLAGS)

pt_a3: A3/periodicTask.c
	$(CC) $< -o $@ $(C_FLAGS) $(L_FLAGS)

.PHONY: clean 

clean:
	rm -f *.c~ 
	rm -f *.o
	rm pt_a1 pt_a2 pt_a3

# Some notes
# $@ represents the left side of the ":"
# $^ represents the right side of the ":"
# $< represents the first item in the dependency list   

