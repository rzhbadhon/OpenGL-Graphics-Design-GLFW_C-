# Compile the code
g++ -fdiagnostics-color=always -I./include ./src/main.cpp ./src/glad.c -o ./build/main.exe -Llib -lglfw3dll -lopengl32 -lgdi32

# If compilation is successful, copy the DLL and run the program
if ($?) {
    cp lib/glfw3.dll build/
    ./build/main.exe
}