# Default target: Compiles the program
all:
	mkdir -p ./src/core/target
	gcc -O2 ./src/core/branch_and_bound.c -fPIC -shared -lm -o ./src/core/target/branch_and_bound.so

# Removes generated binary
clean:
	rm -f ./src/core/target/branch_and_bound.so
