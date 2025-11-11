# This is the repo for Computer Graphics & Multimedia Lab
## Requirements

- C++ compiler
- [GLFW](https://www.glfw.org/)
- [GLAD](https://glad.dav1d.de/)
- OpenGL 3.3 or higher

## Build & Run

1. **Clone the repository:**
   ```sh
   git clone https://github.com/rzhbadhon/OpenGL-Graphics-Design-GLFW_C-.git
   cd OpenGL-Graphics-Design-GLFW_C-
   ```

2. Make sure you have GLFW and GLAD set up in your project.

3. Compile and run:

   win:
	g++.exe -fdiagnostics-color=always -I./include ./src/main.cpp ./src/glad.c -o ./build/main.exe -Llib -lglfw3 -lopengl32 -lgdi32
	./build/main.exe

linux:
	g++ -fdiagnostics-color=always -I./include ./src/main.cpp ./src/glad.c -o ./build/main -Llib -lglfw -lGL -lXrandr -lX11 -lrt -ldl
	./build/main

   *(Adjust the command for your environment and library paths.)*




