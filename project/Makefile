CC =  gcc # Set the compiler
L_FLAGS = -lrt -lm
#C_FLAGS = -g

all: imageProcAlg serialTest
.PHONY: all

# Project compilation
imageProcAlg: imageProcAlg.c
	$(CC) $< -o $@ $(C_FLAGS) $(L_FLAGS)

serialTest: serialTest.c
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

