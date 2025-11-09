#include "glad.h"
#include "glfw3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

// Shader sources with 3D perspective
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec3 color;\n"
"uniform float alpha;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(color, alpha);\n"
"}\0";

// Screen settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Game structures
struct Ball {
    glm::vec3 pos;
    glm::vec3 vel;
    glm::vec3 color;
    float radius;
    float trail[20][3]; // Trail positions
    int trailIndex;
};

struct Target {
    glm::vec3 pos;
    glm::vec3 color;
    float radius;
    bool collected;
    float pulseTimer;
};

struct Particle {
    glm::vec3 pos;
    glm::vec3 vel;
    glm::vec3 color;
    float life;
    float size;
};

// Game state
Ball player;
std::vector<Target> targets;
std::vector<Particle> particles;
glm::vec3 gravity(0.0f, -0.5f, 0.0f);
float cubeRotation = 0.0f;
float cameraRotation = 0.0f;
int score = 0;
int targetCount = 8;
bool gravityFlipped = false;
float timeScale = 1.0f;
float bgColorShift = 0.0f;

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, float deltaTime);
void updateGame(float deltaTime);
void spawnTargets();
void createExplosion(glm::vec3 pos, glm::vec3 color);
void drawCube(unsigned int shaderProgram, unsigned int VAO, glm::mat4 view, glm::mat4 projection);
void drawSphere(unsigned int shaderProgram, unsigned int sphereVAO, int sphereVertexCount, 
                glm::vec3 pos, float radius, glm::vec3 color, glm::mat4 view, glm::mat4 projection);

int main()
{
    srand(time(0));
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "GRAVITY CHAOS 3D", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Compile shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Create cube vertices (wireframe edges)
    float cubeVertices[] = {
        // Bottom face
        -0.8f, -0.8f, -0.8f,  0.8f, -0.8f, -0.8f,
         0.8f, -0.8f, -0.8f,  0.8f, -0.8f,  0.8f,
         0.8f, -0.8f,  0.8f, -0.8f, -0.8f,  0.8f,
        -0.8f, -0.8f,  0.8f, -0.8f, -0.8f, -0.8f,
        // Top face
        -0.8f,  0.8f, -0.8f,  0.8f,  0.8f, -0.8f,
         0.8f,  0.8f, -0.8f,  0.8f,  0.8f,  0.8f,
         0.8f,  0.8f,  0.8f, -0.8f,  0.8f,  0.8f,
        -0.8f,  0.8f,  0.8f, -0.8f,  0.8f, -0.8f,
        // Vertical edges
        -0.8f, -0.8f, -0.8f, -0.8f,  0.8f, -0.8f,
         0.8f, -0.8f, -0.8f,  0.8f,  0.8f, -0.8f,
         0.8f, -0.8f,  0.8f,  0.8f,  0.8f,  0.8f,
        -0.8f, -0.8f,  0.8f, -0.8f,  0.8f,  0.8f,
    };

    unsigned int cubeVBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Create sphere vertices (icosphere approximation)
    std::vector<float> sphereVertices;
    const int segments = 20;
    const int rings = 20;
    for (int i = 0; i <= rings; i++) {
        float theta1 = i * 3.14159f / rings;
        float theta2 = (i + 1) * 3.14159f / rings;
        
        for (int j = 0; j <= segments; j++) {
            float phi1 = j * 2.0f * 3.14159f / segments;
            float phi2 = (j + 1) * 2.0f * 3.14159f / segments;
            
            // Triangle 1
            sphereVertices.push_back(sin(theta1) * cos(phi1));
            sphereVertices.push_back(cos(theta1));
            sphereVertices.push_back(sin(theta1) * sin(phi1));
            
            sphereVertices.push_back(sin(theta2) * cos(phi1));
            sphereVertices.push_back(cos(theta2));
            sphereVertices.push_back(sin(theta2) * sin(phi1));
            
            sphereVertices.push_back(sin(theta2) * cos(phi2));
            sphereVertices.push_back(cos(theta2));
            sphereVertices.push_back(sin(theta2) * sin(phi2));
            
            // Triangle 2
            sphereVertices.push_back(sin(theta1) * cos(phi1));
            sphereVertices.push_back(cos(theta1));
            sphereVertices.push_back(sin(theta1) * sin(phi1));
            
            sphereVertices.push_back(sin(theta2) * cos(phi2));
            sphereVertices.push_back(cos(theta2));
            sphereVertices.push_back(sin(theta2) * sin(phi2));
            
            sphereVertices.push_back(sin(theta1) * cos(phi2));
            sphereVertices.push_back(cos(theta1));
            sphereVertices.push_back(sin(theta1) * sin(phi2));
        }
    }

    unsigned int sphereVBO, sphereVAO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Initialize player ball
    player.pos = glm::vec3(0.0f, 0.0f, 0.0f);
    player.vel = glm::vec3(0.0f, 0.0f, 0.0f);
    player.color = glm::vec3(0.0f, 1.0f, 1.0f);
    player.radius = 0.08f;
    player.trailIndex = 0;
    for (int i = 0; i < 20; i++) {
        player.trail[i][0] = player.trail[i][1] = player.trail[i][2] = 0.0f;
    }

    spawnTargets();

    float lastTime = glfwGetTime();

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        float currentTime = glfwGetTime();
        float deltaTime = (currentTime - lastTime) * timeScale;
        lastTime = currentTime;

        processInput(window, deltaTime);
        updateGame(deltaTime);

        // Dynamic background color
        bgColorShift += deltaTime * 0.5f;
        float r = 0.05f + sin(bgColorShift) * 0.05f;
        float g = 0.05f + sin(bgColorShift * 1.3f) * 0.05f;
        float b = 0.1f + sin(bgColorShift * 0.7f) * 0.05f;
        
        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Setup 3D camera
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), 
                                               (float)SCR_WIDTH / (float)SCR_HEIGHT, 
                                               0.1f, 100.0f);
        
        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.5f));
        view = glm::rotate(view, cameraRotation, glm::vec3(0.0f, 1.0f, 0.0f));
        view = glm::rotate(view, glm::radians(-20.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        // Draw rotating cube wireframe
        drawCube(shaderProgram, cubeVAO, view, projection);

        // Draw player trail (motion blur effect)
        for (int i = 0; i < 20; i++) {
            int idx = (player.trailIndex - i + 20) % 20;
            float alpha = (20 - i) / 20.0f * 0.3f;
            glm::vec3 trailPos(player.trail[idx][0], player.trail[idx][1], player.trail[idx][2]);
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::rotate(model, cubeRotation, glm::vec3(0.5f, 1.0f, 0.3f));
            model = glm::translate(model, trailPos);
            model = glm::scale(model, glm::vec3(player.radius * 0.8f));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(player.color));
            glUniform1f(glGetUniformLocation(shaderProgram, "alpha"), alpha);

            glBindVertexArray(sphereVAO);
            glDrawArrays(GL_TRIANGLES, 0, sphereVertices.size() / 3);
        }

        // Draw player ball with rotation effect
        drawSphere(shaderProgram, sphereVAO, sphereVertices.size() / 3, 
                  player.pos, player.radius, player.color, view, projection);

        // Draw targets with pulse effect
        for (auto& target : targets) {
            if (!target.collected) {
                float pulseSize = target.radius * (1.0f + sin(target.pulseTimer * 5.0f) * 0.2f);
                drawSphere(shaderProgram, sphereVAO, sphereVertices.size() / 3,
                          target.pos, pulseSize, target.color, view, projection);
            }
        }

        // Draw particles
        for (auto& p : particles) {
            drawSphere(shaderProgram, sphereVAO, sphereVertices.size() / 3,
                      p.pos, p.size, p.color, view, projection);
        }

        // Update window title
        char title[100];
        sprintf(title, "GRAVITY CHAOS 3D | Score: %d | Targets: %d/%d | Time: %.1fx", 
                score, targetCount - targets.size(), targetCount, timeScale);
        glfwSetWindowTitle(window, title);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float force = 1.5f;
    
    // Movement controls (relative to camera)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        player.vel.y += force * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        player.vel.y -= force * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        player.vel.x -= force * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        player.vel.x += force * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        player.vel.z -= force * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        player.vel.z += force * deltaTime;

    // Camera rotation
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        cameraRotation -= deltaTime * 2.0f;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        cameraRotation += deltaTime * 2.0f;

    // Gravity flip
    static bool spacePressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
        gravity.y *= -1.0f;
        gravityFlipped = !gravityFlipped;
        createExplosion(player.pos, glm::vec3(1.0f, 1.0f, 0.0f));
        spacePressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
        spacePressed = false;

    // Time control
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        timeScale = 0.3f; // Slow motion
    else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        timeScale = 2.0f; // Fast forward
    else
        timeScale = 1.0f;

    // Reset
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        player.pos = glm::vec3(0.0f, 0.0f, 0.0f);
        player.vel = glm::vec3(0.0f, 0.0f, 0.0f);
        targets.clear();
        particles.clear();
        spawnTargets();
        score = 0;
    }
}

void updateGame(float deltaTime)
{
    // Apply gravity
    player.vel += gravity * deltaTime;

    // Update position
    player.pos += player.vel * deltaTime;

    // Update trail
    player.trail[player.trailIndex][0] = player.pos.x;
    player.trail[player.trailIndex][1] = player.pos.y;
    player.trail[player.trailIndex][2] = player.pos.z;
    player.trailIndex = (player.trailIndex + 1) % 20;

    // Cube boundary collision with bounce
    float boundary = 0.8f - player.radius;
    if (player.pos.x < -boundary) { player.pos.x = -boundary; player.vel.x *= -0.8f; }
    if (player.pos.x > boundary) { player.pos.x = boundary; player.vel.x *= -0.8f; }
    if (player.pos.y < -boundary) { player.pos.y = -boundary; player.vel.y *= -0.8f; }
    if (player.pos.y > boundary) { player.pos.y = boundary; player.vel.y *= -0.8f; }
    if (player.pos.z < -boundary) { player.pos.z = -boundary; player.vel.z *= -0.8f; }
    if (player.pos.z > boundary) { player.pos.z = boundary; player.vel.z *= -0.8f; }

    // Rotate cube
    cubeRotation += deltaTime * 0.3f;

    // Check target collision
    for (auto& target : targets) {
        if (!target.collected) {
            target.pulseTimer += deltaTime;
            float dist = glm::length(player.pos - target.pos);
            if (dist < (player.radius + target.radius)) {
                target.collected = true;
                score += 100;
                createExplosion(target.pos, target.color);
                
                // Respawn if all collected
                if (std::all_of(targets.begin(), targets.end(), 
                    [](const Target& t) { return t.collected; })) {
                    targets.clear();
                    targetCount += 2;
                    spawnTargets();
                }
            }
        }
    }

    // Update particles
    for (auto& p : particles) {
        p.vel += gravity * deltaTime * 0.5f;
        p.pos += p.vel * deltaTime;
        p.life -= deltaTime;
        p.size *= 0.98f;
    }

    particles.erase(std::remove_if(particles.begin(), particles.end(),
                    [](const Particle& p) { return p.life <= 0.0f; }), particles.end());
}

void spawnTargets()
{
    for (int i = 0; i < targetCount; i++) {
        Target target;
        target.pos = glm::vec3(
            ((float)rand() / RAND_MAX) * 1.2f - 0.6f,
            ((float)rand() / RAND_MAX) * 1.2f - 0.6f,
            ((float)rand() / RAND_MAX) * 1.2f - 0.6f
        );
        target.radius = 0.06f;
        target.color = glm::vec3(
            0.5f + (float)rand() / RAND_MAX * 0.5f,
            0.5f + (float)rand() / RAND_MAX * 0.5f,
            0.5f + (float)rand() / RAND_MAX * 0.5f
        );
        target.collected = false;
        target.pulseTimer = 0.0f;
        targets.push_back(target);
    }
}

void createExplosion(glm::vec3 pos, glm::vec3 color)
{
    for (int i = 0; i < 50; i++) {
        Particle p;
        p.pos = pos;
        float theta = ((float)rand() / RAND_MAX) * 6.28f;
        float phi = ((float)rand() / RAND_MAX) * 3.14f;
        float speed = ((float)rand() / RAND_MAX) * 0.5f + 0.2f;
        p.vel = glm::vec3(
            sin(phi) * cos(theta) * speed,
            sin(phi) * sin(theta) * speed,
            cos(phi) * speed
        );
        p.color = color;
        p.life = 1.0f + (float)rand() / RAND_MAX;
        p.size = 0.03f;
        particles.push_back(p);
    }
}

void drawCube(unsigned int shaderProgram, unsigned int VAO, glm::mat4 view, glm::mat4 projection)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, cubeRotation, glm::vec3(0.5f, 1.0f, 0.3f));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glm::vec3 cubeColor = glm::vec3(0.3f, 0.7f, 1.0f);
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(cubeColor));
    glUniform1f(glGetUniformLocation(shaderProgram, "alpha"), 0.6f);

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, 24);
}

void drawSphere(unsigned int shaderProgram, unsigned int sphereVAO, int sphereVertexCount,
                glm::vec3 pos, float radius, glm::vec3 color, glm::mat4 view, glm::mat4 projection)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, cubeRotation, glm::vec3(0.5f, 1.0f, 0.3f));
    model = glm::translate(model, pos);
    model = glm::scale(model, glm::vec3(radius));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(shaderProgram, "alpha"), 1.0f);

    glBindVertexArray(sphereVAO);
    glDrawArrays(GL_TRIANGLES, 0, sphereVertexCount);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}