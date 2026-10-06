#pragma once
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace enzo::gfx {

/// @brief The camera a viewport looks through, orbiting a centre point.
class ViewportCamera
{
  public:
    /// @brief Turns the camera around the centre by a pointer drag in pixels.
    void orbit(glm::vec2 delta);

    /// @brief Slides the camera and its centre across the view by a pointer drag in pixels.
    void pan(glm::vec2 delta);

    /// @brief Moves the camera toward the centre for a negative amount and away for a positive one.
    ///
    /// @note Close to the centre the camera keeps moving forward and carries the centre with it.
    void dolly(float amount);

    /// @brief Returns the camera position in world space.
    glm::vec3 getPosition() const;

    /// @brief Returns the matrix taking world space to view space.
    glm::mat4 getViewMatrix() const;

    /// @brief Returns the matrix taking view space to clip space, with depth from 0 to 1.
    ///
    /// @param aspect the view width divided by its height.
    glm::mat4 getProjectionMatrix(float aspect) const;

    bool operator==(const ViewportCamera&) const = default;

  private:
    /// @brief Returns the unit direction the camera looks along, from its position to the centre.
    glm::vec3 getForward() const;

    glm::vec3 center_{0.f, 0.f, 0.f};
    float distance_ = 15.f;
    /// @brief The angle around the vertical axis, 0 looking down negative z.
    float yaw_ = glm::radians(-135.f);
    /// @brief The angle above the horizontal plane.
    float pitch_ = glm::radians(20.f);
};

} // namespace enzo::gfx
