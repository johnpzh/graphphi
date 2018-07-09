
DIRS = $(wildcard */.)

.PHONY: all clean $(DIRS)

all: $(DIRS)

$(DIRS): 
	$(MAKE) -C $@

clean:
	for dir in $(DIRS); do \
		(cd $$dir && $(MAKE) clean; ) \
	done

