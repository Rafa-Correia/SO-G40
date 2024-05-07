# Directories
SRC_DIR_CLIENT = client/src
SRC_DIR_SERVER = server/src
INCLUDE_DIR_SERVER = server/include
INCLUDE_DIR_UTILS = utils/include
SRC_DIR_UTILS = utils/src
OBJ_DIR = obj
BIN_DIR = bin
TMP_BIN = tmp

# Files
CLIENT_SRC = $(SRC_DIR_CLIENT)/client.c
SERVER_SRC = $(SRC_DIR_SERVER)/server.c $(SRC_DIR_SERVER)/request_handler.c
UTILS_SRC = $(SRC_DIR_UTILS)/utils.c

CLIENT_OBJ = $(OBJ_DIR)/client.o
SERVER_OBJ = $(OBJ_DIR)/server.o $(OBJ_DIR)/request_handler.o
UTILS_OBJ = $(OBJ_DIR)/utils.o

# Output binaries
CLIENT_BIN = $(BIN_DIR)/client
SERVER_BIN = $(BIN_DIR)/orchestrator

# Compiler settings
CC = gcc
CFLAGS = -I$(INCLUDE_DIR_SERVER) -I$(INCLUDE_DIR_UTILS)

# Targets
all: $(CLIENT_BIN) $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_OBJ) $(UTILS_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

$(SERVER_BIN): $(SERVER_OBJ) $(UTILS_OBJ)
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(TMP_BIN)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR_CLIENT)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR_SERVER)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR_UTILS)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(TMP_BIN)

.PHONY: all clean
