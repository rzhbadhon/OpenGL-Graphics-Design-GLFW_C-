CXX = g++
CXXFLAGS = -fdiagnostics-color=always -I./include
LDFLAGS = -Llib -lglfw3dll -lopengl32 -lgdi32
SRC = ./src/main.cpp ./src/glad.c
OUT = ./build/main.exe

win:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)
	cp lib/glfw3.dll build/
