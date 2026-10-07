#include "Graphics/ViewportCamera.h"
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace enzo::gfx {

namespace {

constexpr glm::vec3 worldUp{0.f, 1.f, 0.f};

// Radians turned per pixel dragged.
constexpr float orbitSpeed = 0.01f;

// Stops short of straight up and down, where the view would flip.
const float maxPitch = glm::radians(89.f);

// World units moved per pixel at a distance of 1.
constexpr float panSpeed = 0.0015f;

// Fraction of the distance moved per unit of dolly.
constexpr float dollySpeed = 0.1f;

// The closest the camera gets to the centre.
constexpr float minDistance = 1.f;

const float fieldOfView = glm::radians(45.f);
constexpr float nearPlane = 0.1f;
constexpr float farPlane = 1000.f;

// Returns the factor a movement scales by, the distance but never less than 1.
float getDistanceScale(float distance) { return std::max(distance, 1.f); }

} // namespace

void ViewportCamera::orbit(glm::vec2 delta)
{
    yaw_ -= delta.x * orbitSpeed;
    pitch_ = std::clamp(pitch_ + delta.y * orbitSpeed, -maxPitch, maxPitch);
}

void ViewportCamera::pan(glm::vec2 delta)
{
    const glm::vec3 forward = getForward();
    const glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    const glm::vec3 up = glm::cross(right, forward);

    // Moves the centre against the drag so the scene follows the pointer.
    const float speed = panSpeed * getDistanceScale(distance_);
    center_ += (up * delta.y - right * delta.x) * speed;
}

void ViewportCamera::dolly(float amount)
{
    const float step = amount * dollySpeed * getDistanceScale(distance_);
    const float targetDistance = distance_ + step;
    if (targetDistance >= minDistance)
    {
        distance_ = targetDistance;
        return;
    }

    // Moves the centre forward by the part of the step past the closest distance.
    center_ += getForward() * (minDistance - targetDistance);
    distance_ = minDistance;
}

void ViewportCamera::placeAt(const glm::mat4& transform)
{
    const glm::vec3 position{transform[3]};
    const glm::vec3 forward = glm::normalize(-glm::vec3(transform[2]));

    // Solves getForward for the yaw and pitch that give this direction.
    yaw_ = std::atan2(-forward.x, -forward.z);
    pitch_ = std::clamp(std::asin(-forward.y), -maxPitch, maxPitch);
    center_ = position + getForward() * distance_;
}

glm::vec3 ViewportCamera::getForward() const
{
    const glm::vec3 centerToCamera{
        std::cos(pitch_) * std::sin(yaw_),
        std::sin(pitch_),
        std::cos(pitch_) * std::cos(yaw_),
    };
    return -centerToCamera;
}

glm::vec3 ViewportCamera::getPosition() const { return center_ - getForward() * distance_; }

glm::mat4 ViewportCamera::getViewMatrix() const
{
    return glm::lookAtRH(getPosition(), center_, worldUp);
}

glm::mat4 ViewportCamera::getProjectionMatrix(float aspect) const
{
    return glm::perspectiveRH_ZO(fieldOfView, aspect, nearPlane, farPlane);
}

} // namespace enzo::gfx
