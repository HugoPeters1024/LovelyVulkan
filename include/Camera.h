#include "precomp.h"

namespace lv {

class Camera {
public:
    Camera(GLFWwindow* window);

    void update();
    glm::vec3 getViewDir() const;
    glm::mat4 getViewMatrix() const;
private:
    GLFWwindow* window;
    glm::vec3 eye;
    float theta, phi;
};

}
