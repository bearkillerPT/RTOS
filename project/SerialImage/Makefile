CC =  gcc # Set the compiler
L_FLAGS = -lrt -lm
#C_FLAGS = -g

all: serialImage
.PHONY: all

# Project compilation

serialImage: serialImage.c
	$(CC) $< -o $@ $(C_FLAGS) $(L_FLAGS)


.PHONY: clean 

clean:
	rm -f *.c~ 
	rm -f *.o
	rm serialImage

# Some notes
# $@ represents the left side of the ":"
# $^ represents the right side of the ":"
# $< represents the first item in the dependency list   

