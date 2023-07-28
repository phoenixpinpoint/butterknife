DEP_DIR := deps

# Find all /deps foldrers and build a dynamic -I flag for each.
INCLUDES := $(wildcard $(DEP_DIR)/**)
INCLUDES := $(foreach dir,$(INCLUDES), -I$(dir))


test:
	gcc $(INCLUDES) test.c -o test

clean:
	rm -rf ./test