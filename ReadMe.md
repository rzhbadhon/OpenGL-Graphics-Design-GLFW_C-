# 🎨 Cyan Window – GLFW + OpenGL

A super simple OpenGL example that opens a window with a cyan background and a custom title:  
**Razibul Hasan Badhon**  
Press **R** (for "Razibul") or **ESC** to close the window.

---

## ✨ Features

- 🟦 **Cyan background** – bright and eye-catching
- 📝 **Custom window title** – your name at the top
- ⌨ **Simple controls** – press R or ESC to exit
- 🪶 **Minimal & beginner-friendly code**

---

## ⚙ Build & Run

1. **Clone the repository:**
   ```sh
   git clone https://github.com/your-username/Cyan-Window-OpenGL.git
   cd Cyan-Window-OpenGL
   ```

2. **Make sure you have GLFW and GLAD set up in your project.**

3. **Compile and run (Windows example):**
   ```sh
   g++ src/main.cpp src/glad.c -Iinclude -o cyan_window -lglfw3 -lopengl32 -lgdi32
   .\cyan_window
   ```
   *(Adjust the command for your environment and library paths.)*

---

## 📂 Project Structure

```
Graphics/
│
├── include/       # Header files (GLFW, GLAD, etc.)
├── src/           # Source files
│   ├── main.cpp   # Entry point
│   └── glad.c     # GLAD loader
└── README.md
```

---

## ⌨ Controls

- **R** or **ESC**: Exit the window

---

## 📜 License

This project is licensed under the MIT License – feel free to modify and share.

---
## Output

![Screenshot](screenshot.png)
