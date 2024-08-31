BINUTILS_BUILT_DIR=../../target/built
CFLAG_INCLUDES=-I$(BINUTILS_BUILT_DIR)/include/
CFLAGS=-ggdb2 -fPIC
LDFLAGS=-Wl,-rpath,$(BINUTILS_BUILT_DIR)/lib -L$(BINUTILS_BUILT_DIR)/lib
LIBS=-lopcodes -lz -lzstd -ldl -lm -lbfd -liberty -lsframe 
TARGETS=test_binary test_buffer_mep test_buffer_x86_64

all: $(TARGETS)

.IGNORE: clean
clean:
	@rm -f $(TARGETS)

%: src/%.c
	gcc -v -o $@ $^ $(CFLAG_INCLUDES) $(CFLAGS) $(LDFLAGS) $(LIBS)