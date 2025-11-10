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

// Shader sources (unchanged)
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

struct Hazard {
    glm::vec3 pos;
    glm::vec3 color;
    float radius;
    float pulseTimer;
};

// Game state
Ball player;
std::vector<Target> targets;
std::vector<Particle> particles;
std::vector<Hazard> hazards;
glm::vec3 gravity(0.0f, -0.6f, 0.0f); // Stronger gravity
int score = 0;
int level = 1;

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void updateGame(float deltaTime);
void spawnLevel(int level);
void createExplosion(glm::vec3 pos, glm::vec3 color, int count);
void drawCube(unsigned int shaderProgram, unsigned int VAO, glm::mat4 view, glm::mat4 projection);
void drawSphere(unsigned int shaderProgram, unsigned int sphereVAO, int sphereVertexCount,
                glm::vec3 pos, float radius, glm::vec3 color, float alpha, glm::mat4 view, glm::mat4 projection);

// Helper function to reset the game
void resetGame() {
    player.pos = glm::vec3(0.0f, 0.0f, 0.0f);
    player.vel = glm::vec3(0.0f, 0.0f, 0.0f);
    gravity = glm::vec3(0.0f, -0.6f, 0.0f);
    targets.clear();
    particles.clear();
    hazards.clear();
    spawnLevel(level);
    score = (level - 1) * 100; // Keep score from previous levels
}

int main()
{
    srand(time(0));

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Gravity Box", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Compile shaders (unchanged)
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

    // Create cube vertices (unchanged)
    float cubeVertices[] = {
        -0.8f, -0.8f, -0.8f,  0.8f, -0.8f, -0.8f,
         0.8f, -0.8f, -0.8f,  0.8f, -0.8f,  0.8f,
         0.8f, -0.8f,  0.8f, -0.8f, -0.8f,  0.8f,
        -0.8f, -0.8f,  0.8f, -0.8f, -0.8f, -0.8f,
        -0.8f,  0.8f, -0.8f,  0.8f,  0.8f, -0.8f,
         0.8f,  0.8f, -0.8f,  0.8f,  0.8f,  0.8f,
         0.8f,  0.8f,  0.8f, -0.8f,  0.8f,  0.8f,
        -0.8f,  0.8f,  0.8f, -0.8f,  0.8f, -0.8f,
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

    // Create sphere vertices (unchanged)
    std::vector<float> sphereVertices;
    const int segments = 20;
    const int rings = 20;
    for (int i = 0; i <= rings; i++) {
        float theta1 = i * 3.14159f / rings;
        float theta2 = (i + 1) * 3.14159f / rings;
        for (int j = 0; j <= segments; j++) {
            float phi1 = j * 2.0f * 3.14159f / segments;
            float phi2 = (j + 1) * 2.0f * 3.14159f / segments;
            sphereVertices.push_back(sin(theta1) * cos(phi1));
            sphereVertices.push_back(cos(theta1));
            sphereVertices.push_back(sin(theta1) * sin(phi1));
            sphereVertices.push_back(sin(theta2) * cos(phi1));
            sphereVertices.push_back(cos(theta2));
            sphereVertices.push_back(sin(theta2) * sin(phi1));
            sphereVertices.push_back(sin(theta2) * cos(phi2));
            sphereVertices.push_back(cos(theta2));
            sphereVertices.push_back(sin(theta2) * sin(phi2));
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
    player.color = glm::vec3(0.0f, 1.0f, 1.0f);
    player.radius = 0.05f;

    spawnLevel(level);
    resetGame();

    float lastTime = glfwGetTime();

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // Cap delta time to prevent physics explosions
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        processInput(window);
        updateGame(deltaTime);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Setup 3D camera - STATIC
        glm::mat4 projection = glm::perspective(glm::radians(60.0f),
                                               (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                               0.1f, 100.0f);
        // Camera is now fixed, looking at the center from the front
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
                                   glm::vec3(0.0f, 0.0f, 0.0f),
                                   glm::vec3(0.0f, 1.0f, 0.0f));

        // Draw static cube wireframe
        drawCube(shaderProgram, cubeVAO, view, projection);

        // Draw player ball
        drawSphere(shaderProgram, sphereVAO, sphereVertices.size() / 3,
                   player.pos, player.radius, player.color, 1.0f, view, projection);

        // Draw targets with pulse effect
        for (auto& target : targets) {
            if (!target.collected) {
                float pulseSize = target.radius * (1.0f + sin(target.pulseTimer * 5.0f) * 0.2f);
                drawSphere(shaderProgram, sphereVAO, sphereVertices.size() / 3,
                           target.pos, pulseSize, target.color, 1.0f, view, projection);
            }
        }

        // Draw hazards
        for (auto& hazard : hazards) {
            float pulseSize = hazard.radius * (1.0f + cos(hazard.pulseTimer * 3.0f) * 0.15f);
             drawSphere(shaderProgram, sphereVAO, sphereVertices.size() / 3,
                       hazard.pos, pulseSize, hazard.color, 1.0f, view, projection);
        }

        // Draw particles
        for (auto& p : particles) {
            float alpha = p.life / 2.0f; // Fade out
            drawSphere(shaderProgram, sphereVAO, sphereVertices.size() / 3,
                       p.pos, p.size, p.color, alpha, view, projection);
        }

        // Update window title
        int targetsLeft = 0;
        for(auto& t : targets) if(!t.collected) targetsLeft++;
        
        char title[100];
        sprintf(title, "GRAVITY BOX | Level: %d | Score: %d | Targets Left: %d",
                level, score, targetsLeft);
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

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float moveSpeed = 1.0f;
    
    // Horizontal Movement
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        player.vel.x = -moveSpeed;
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        player.vel.x = moveSpeed;
    else
        player.vel.x = 0; // Stop immediately

    // Gravity flip
    static bool spacePressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
        gravity.y *= -1.0f;
        // Add a small opposite velocity to "jump" off the surface
        player.vel.y = gravity.y * 0.1f; 
        createExplosion(player.pos, glm::vec3(1.0f, 1.0f, 0.0f), 20);
        spacePressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
        spacePressed = false;

    // Reset
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        level = 1;
        resetGame();
    }
}

void updateGame(float deltaTime)
{
    // Apply gravity
    player.vel += gravity * deltaTime;

    // Update position
    player.pos += player.vel * deltaTime;

    // Clamp to 2D
    player.pos.z = 0.0f;

    // Cube boundary collision (now 2D)
    float boundary = 0.8f - player.radius;
    
    // Side walls
    if (player.pos.x < -boundary) { player.pos.x = -boundary; player.vel.x = 0; }
    if (player.pos.x > boundary) { player.pos.x = boundary; player.vel.x = 0; }
    
    // Top/Bottom "floor"
    // We stop the velocity but don't check for hazard collision here
    if (player.pos.y < -boundary) { player.pos.y = -boundary; player.vel.y = 0; }
    if (player.pos.y > boundary) { player.pos.y = boundary; player.vel.y = 0; }

    // Check hazard collision
    for (auto& hazard : hazards) {
        hazard.pulseTimer += deltaTime;
        float dist = glm::length(player.pos - hazard.pos);
        if (dist < (player.radius + hazard.radius)) {
            createExplosion(player.pos, player.color, 50);
            resetGame(); // Game over, reset level
            return; // Stop update for this frame
        }
    }

    // Check target collision
    bool allCollected = true;
    for (auto& target : targets) {
        if (!target.collected) {
            allCollected = false;
            target.pulseTimer += deltaTime;
            float dist = glm::length(player.pos - target.pos);
            if (dist < (player.radius + target.radius)) {
                target.collected = true;
                score += 10;
                createExplosion(target.pos, target.color, 30);
            }
        }
    }

    // Check for level complete
    if (allCollected && !targets.empty()) {
        level++;
        score += 100; // Level complete bonus
        spawnLevel(level); // Go to next level
    }

    // Update particles
    for (auto& p : particles) {
        p.vel += gravity * deltaTime * 0.3f; // Particles are slightly affected by gravity
        p.pos += p.vel * deltaTime;
        p.life -= deltaTime;
        p.size *= 0.98f;
    }

    particles.erase(std::remove_if(particles.begin(), particles.end(),
                    [](const Particle& p) { return p.life <= 0.0f; }), particles.end());
}

void spawnLevel(int level)
{
    // Clear old objects
    targets.clear();
    hazards.clear();
    particles.clear();

    // Reset player position
    player.pos = glm::vec3(0.0f, 0.0f, 0.0f);
    player.vel = glm::vec3(0.0f, 0.0f, 0.0f);
    gravity = glm::vec3(0.0f, -0.6f, 0.0f); // Reset gravity

    float boundary = 0.8f;

    // Add hazards to top and bottom
    for (float x = -boundary; x <= boundary; x += 0.15f) {
        Hazard topHazard, bottomHazard;
        topHazard.pos = glm::vec3(x, boundary - 0.05f, 0.0f);
        topHazard.color = glm::vec3(1.0f, 0.2f, 0.2f);
        topHazard.radius = 0.04f;
        topHazard.pulseTimer = 0.0f;
        
        bottomHazard.pos = glm::vec3(x, -boundary + 0.05f, 0.0f);
        bottomHazard.color = glm::vec3(1.0f, 0.2f, 0.2f);
        bottomHazard.radius = 0.04f;
        bottomHazard.pulseTimer = 0.0f;

        hazards.push_back(topHazard);
        hazards.push_back(bottomHazard);
    }
    
    // Spawn targets
    int targetCount = 2 + level; // Increase targets with level
    float safeZone = 0.6f; // Spawn away from walls
    
    for (int i = 0; i < targetCount; i++) {
        Target target;
        target.pos = glm::vec3(
            ((float)rand() / RAND_MAX) * safeZone * 2.0f - safeZone,
            ((float)rand() / RAND_MAX) * safeZone * 2.0f - safeZone,
            0.0f // Ensure Z is 0
        );
        target.radius = 0.04f;
        target.color = glm::vec3(0.2f, 1.0f, 0.2f); // Green
        target.collected = false;
        target.pulseTimer = (float)rand() / RAND_MAX * 5.0f;
        targets.push_back(target);
    }
}

void createExplosion(glm::vec3 pos, glm::vec3 color, int count)
{
    for (int i = 0; i < count; i++) {
        Particle p;
        p.pos = pos;
        float angle = ((float)rand() / RAND_MAX) * 6.28f; // 2D circle
        float speed = ((float)rand() / RAND_MAX) * 1.0f + 0.2f;
        p.vel = glm::vec3(
            cos(angle) * speed,
            sin(angle) * speed,
            0.0f // 2D only
        );
        p.color = color;
        p.life = 1.0f + (float)rand() / RAND_MAX;
        p.size = 0.03f;
        particles.push_back(p);
    }
}

void drawCube(unsigned int shaderProgram, unsigned int VAO, glm::mat4 view, glm::mat4 projection)
{
    // Draw a static, non-rotating cube
    glm::mat4 model = glm::mat4(1.0f);

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
                glm::vec3 pos, float radius, glm::vec3 color, float alpha, glm::mat4 view, glm::mat4 projection)
{
    // Model matrix now only translates and scales. No rotation.
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, pos);
    model = glm::scale(model, glm::vec3(radius));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(shaderProgram, "alpha"), alpha);

    glBindVertexArray(sphereVAO);
    glDrawArrays(GL_TRIANGLES, 0, sphereVertexCount);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}