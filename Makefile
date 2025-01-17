SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

LIBDPF_SRC_DIR = $(SRC_DIR)/libdpf
LIBDPF_OBJ_DIR = $(OBJ_DIR)/libdpf

DATA_SRC_DIR = $(SRC_DIR)/data
DATA_OBJ_DIR = $(OBJ_DIR)/data

BENCHMARK_SRC_DIR = $(SRC_DIR)/benchmark
BENCHMARK_OBJ_DIR = $(OBJ_DIR)/benchmark

TEST_SRC_DIR = test
TEST_OBJ_DIR = $(OBJ_DIR)/test
TEST_BIN_DIR = $(BIN_DIR)/test


SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

LIBDPF_SRC := $(wildcard $(LIBDPF_SRC_DIR)/*.cpp)
LIBDPF_OBJ := $(LIBDPF_SRC:$(LIBDPF_SRC_DIR)/%.cpp=$(LIBDPF_OBJ_DIR)/%.o)

DATA_SRC := $(wildcard $(DATA_SRC_DIR)/*.cpp)
DATA_OBJ := $(DATA_SRC:$(DATA_SRC_DIR)/%.cpp=$(DATA_OBJ_DIR)/%.o)

BENCHMARK_SRC := $(wildcard $(BENCHMARK_SRC_DIR)/*.cpp)
BENCHMARK_OBJ := $(BENCHMARK_SRC:$(BENCHMARK_SRC_DIR)/%.cpp=$(BENCHMARK_OBJ_DIR)/%.o)

TEST_SRC := $(wildcard $(TEST_SRC_DIR)/*.cpp)
TEST_OBJ := $(TEST_SRC:$(TEST_SRC_DIR)/%.cpp=$(TEST_OBJ_DIR)/%.o)
TEST_BIN := $(TEST_OBJ:$(TEST_OBJ_DIR)/%.o=$(TEST_BIN_DIR)/%)


CC = clang++
CFLAGS = -Wall -O3 -g -Werror --std=c++17 -DSHA256_ASM -DAES_ENC
CPPFLAGS = -I/opt/local/include -Iinclude -Iinclude/libdpf -Iinclude/data -Iinclude/benchmark -maes -msse2 -fopenmp -flto -mavx2
LDFLAGS = -L/opt/local/lib -Llib -pthread -lboost_program_options -flto
LDLIBS = -lcrypto
# -lboost_program_options-mt -lpbc -lgmp
# -lcryptopp
# -mmacosx-version-min=11.2

.PHONY: all clean

# all: $(EXE)

# link src/libdpf and src
# $(EXE): $(LIBDPF_OBJ) $(OBJ) $(TEST_OBJ) | $(BIN_DIR)
# 	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

all: $(TEST_BIN)

# link test
$(TEST_BIN): $(LIBDPF_OBJ) $(DATA_OBJ) $(BENCHMARK_OBJ) $(OBJ) $(TEST_OBJ) | $(TEST_BIN_DIR)
	$(CC) $(LDFLAGS) $(LIBDPF_OBJ) $(DATA_OBJ) $(BENCHMARK_OBJ) $(OBJ) $(TEST_OBJ_DIR)/$(notdir $@).o $(LDLIBS) -o $@

# compile src/libdpf
$(LIBDPF_OBJ_DIR)/%.o: $(LIBDPF_SRC_DIR)/%.cpp | $(LIBDPF_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# compile src/data
$(DATA_OBJ_DIR)/%.o: $(DATA_SRC_DIR)/%.cpp | $(DATA_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# compile src/benchmark
$(BENCHMARK_OBJ_DIR)/%.o: $(BENCHMARK_SRC_DIR)/%.cpp | $(BENCHMARK_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# compile src
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# compile test
$(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.cpp | $(TEST_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# libdpf: $(SRC_DIR)/libdpf/aes.h $(SRC_DIR)/libdpf/libdpf.h $(SRC_DIR)/libdpf/libdpf.c $(SRC_DIR)/libdpf/block.h $(SRC_DIR)/libdpf/block.c
# 	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) -c $< -o $@

# dpf: $(SRC_DIR)/%.cpp $(SRC_DIR)/%.h
# 	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) -c $< -o $@ libdpf.o

$(BIN_DIR) $(LIBDPF_OBJ_DIR) $(DATA_OBJ_DIR) $(BENCHMARK_OBJ_DIR) $(OBJ_DIR) $(TEST_OBJ_DIR) $(TEST_BIN_DIR):
	mkdir -p $@

clean:
	@$(RM) -rv $(BIN_DIR) $(LIBDPF_OBJ_DIR) $(DATA_OBJ_DIR) $(BENCHMARK_OBJ_DIR) $(OBJ_DIR) $(TEST_OBJ_DIR) $(TEST_BIN_DIR) # The @ disables the echoing of the command