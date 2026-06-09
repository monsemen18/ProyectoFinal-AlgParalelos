# ─────────────────────────────────────────────
#  Raytracer en C con OpenMP
# ─────────────────────────────────────────────

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -Iinclude -fopenmp -lm
LDFLAGS = -fopenmp -lm -lcjson

SRC_DIR = src
OBJ_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = raytracer

.PHONY: all clean run bench serial

all: $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "✓ Compilado: $(TARGET)"

# Render normal (paralelo, todos los CPUs disponibles)
run: $(TARGET)
	./$(TARGET) -s scene.json -o output.ppm

# Benchmark: serial vs paralelo con reporte de speedup
bench: $(TARGET)
	./$(TARGET) -s scene.json -o output.ppm --bench

# Solo modo serial
serial: $(TARGET)
	./$(TARGET) -s scene.json -o output_serial.ppm --serial

# Render con N hilos específico
threads: $(TARGET)
	./$(TARGET) -s scene.json -o output.ppm -t $(N)

clean:
	rm -rf $(OBJ_DIR) $(TARGET) *.ppm
