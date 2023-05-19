#include <GLFW/glfw3.h>
#include <math.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define RING_RADIUS 50.0f
#define RING_SPEED 10.0f

void drawRing(float radius) {
    glBegin(GL_LINE_LOOP);
    int i;
    for (i = 0; i < 360; i++) {
        float angle = i * (3.14159f / 180.0f);
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();
}

void display(GLFWwindow* window) {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);

    // Set the viewport and projection matrix
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-WINDOW_WIDTH / 2, WINDOW_WIDTH / 2, -WINDOW_HEIGHT / 2, WINDOW_HEIGHT / 2, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    // Translate to the center of the window
    glTranslatef(0.0f, 0.0f, 0.0f);

    // Set the line width
    glLineWidth(2.0f);

    // Get the current time
    float currentTime = glfwGetTime();

    // Calculate the radius of the ring based on the elapsed time
    float radius = RING_RADIUS + (RING_SPEED * currentTime);

    // Draw the ring
    drawRing(radius);

    // Swap buffers
    glfwSwapBuffers(window);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        return -1;
    }

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Time-Based Animation", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Set the viewport and projection matrix
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-WINDOW_WIDTH / 2, WINDOW_WIDTH / 2, -WINDOW_HEIGHT / 2, WINDOW_HEIGHT / 2, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    // Set the framebuffer size callback
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Render here
        display(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Clean up
    glfwTerminate();

    return 0;
}
