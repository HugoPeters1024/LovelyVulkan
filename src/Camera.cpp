#include "Camera.h"

namespace lv {

Camera::Camera(GLFWwindow* window) : window(window) {
    eye = glm::vec3(0.0f);
    phi = 0.0f;
    theta = glm::pi<float>() / 2.0f;
}

void Camera::update() {
    const float moveSpeed = 0.02f;
    const float rotSpeed = 0.01f;
    const glm::vec3 viewDir = getViewDir();
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) eye += moveSpeed * viewDir;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) eye -= moveSpeed * viewDir;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) theta += rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) theta -= rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) phi -= rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) phi += rotSpeed;

}

glm::vec3 Camera::getViewDir() const {
    return glm::vec3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(eye, eye + getViewDir(), glm::vec3(0,1,0));
}

}
