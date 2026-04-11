#include "Gui/Viewport/GLCamera.h"
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <QOpenGLFunctions>

void printMatrix(const glm::mat4& matrix) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            std::cout << matrix[i][j] << " ";
            }
            std::cout << std::endl;
        }
}
void printVec(const glm::vec3& vector) {
    for (int i = 0; i < 3; ++i) {
        std::cout << vector[i] << " ";
    }
    std::cout << std::endl;
}

void GLCamera::changeCenter(float x, float y, float z)
{
    camCenter_.x += x;
    camCenter_.y += y;
    camCenter_.z += z;

    viewMatrix_ = glm::lookAt(
        camPos_,
        camCenter_,
        camUp_
    );
}

void GLCamera::setCenter(float x, float y, float z)
{
    camCenter_.x = x;
    camCenter_.y = y;
    camCenter_.z = z;

    viewMatrix_ = glm::lookAt(
        camPos_,
        camCenter_,
        camUp_
    );
}

glm::vec3 GLCamera::getUp()
{
    return glm::normalize(glm::cross(getForward(), getRight()));
}


GLCamera::GLCamera()
: GLCamera(0.0f, 0.0f, 10.0f)
{
}

GLCamera::GLCamera(float posX, float posY, float posZ)
{
    setPos(posX, posY, posZ);
    viewMatrix_ = glm::lookAt(
        camPos_,
        camCenter_,
        camUp_
    );
}


glm::mat4 GLCamera::getViewMatrix()
{
    return viewMatrix_;
}

void GLCamera::setView(glm::vec3 pos, glm::vec3 center, glm::vec3 up)
{
    camPos_ = pos;
    camCenter_ = center;
    camUp_ = up;

    viewMatrix_ = glm::lookAt(
        camPos_,
        camCenter_,
        camUp_
    );
}

void GLCamera::setPos(float x, float y, float z)
{
    camPos_.x = x;
    camPos_.y = y;
    camPos_.z = z;
}

void GLCamera::movePos(float x, float y, float z)
{
    camPos_.x += x;
    camPos_.y += y;
    camPos_.z += z;
}


void GLCamera::rotateAroundCenter(float angle, glm::vec3 axis)
{
    glm::mat4 rotMatrix = glm::rotate(glm::mat4(1.0f), angle, axis);

    axis = glm::normalize(axis);
    // build a quaternion for this rotation
    glm::quat q = glm::angleAxis(angle, axis);
    // rotate the position vector
    camPos_ = q * camPos_;

    viewMatrix_ = glm::lookAt(
        camPos_,
        camCenter_,
        camUp_
    );

}

void GLCamera::changeRadius(float delta)
{
    glm::vec3 centerDir = glm::normalize(camPos_-camCenter_);
    camPos_+=centerDir*delta;

    viewMatrix_ = glm::lookAt(
        camPos_,
        camCenter_,
        camUp_
    );

}

glm::vec3 GLCamera::getPos()
{
    return camPos_;
};

glm::vec3 GLCamera::getRight()
{
    return glm::cross(getForward(), camUp_);
};

glm::vec3 GLCamera::getForward()
{
    return glm::normalize(camCenter_-camPos_);
};

void GLCamera::setUniform(uint uniformLocation)
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(getViewMatrix()));
}
