# Nombre del ejecutable
TARGET = ecosim

# Archivo fuente
SRC = ecosim.cpp

# Compilador
CXX = g++

# Flags de compilación
CXXFLAGS = -O2

# Flags de FLTK
FLTK_FLAGS = $(shell fltk-config --cxxflags --ldflags)

# Librerías adicionales
LIBS = -lm

# Regla por defecto
all: $(TARGET)

# Cómo compilar el ejecutable
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(FLTK_FLAGS) $(LIBS)

# Ejecutar el programa
run: $(TARGET)
	./$(TARGET)
	gnuplot plot_ecosim.gnu

# Grafica resultados
plot:
	gnuplot plot_ecosim.gnuplot
	
# Limpiar archivos generados
clean:
	rm -f $(TARGET)

# Evitar conflictos con archivos llamados igual
.PHONY: all run clean plot
