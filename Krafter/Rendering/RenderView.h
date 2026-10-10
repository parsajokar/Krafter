#pragma once

#include <array>

#include "glm/glm.hpp"

namespace Krafter {

class Frustum {
public:
    static Frustum FromViewProjection(const glm::mat4& viewProjection)
    {
        const glm::mat4 m = glm::transpose(viewProjection);
        Frustum frustum;
        frustum.m_Planes = {
            m[3] + m[0],
            m[3] - m[0],
            m[3] + m[1],
            m[3] - m[1],
            m[2],
            m[3] - m[2],
        };
        return frustum;
    }

    bool IntersectsBox(const glm::vec3& min, const glm::vec3& max) const
    {
        for (const glm::vec4& plane : m_Planes) {
            const glm::vec3 corner(
                plane.x >= 0.0f ? max.x : min.x,
                plane.y >= 0.0f ? max.y : min.y,
                plane.z >= 0.0f ? max.z : min.z);
            if (glm::dot(glm::vec3(plane), corner) + plane.w < 0.0f) {
                return false;
            }
        }
        return true;
    }

private:
    std::array<glm::vec4, 6> m_Planes = {};
};

struct RenderView {
    glm::vec3 cameraPosition;
    glm::mat4 viewProjection;
    Frustum frustum;

    RenderView(const glm::vec3& cameraPosition, const glm::mat4& relativeViewProjection)
        : cameraPosition(cameraPosition)
        , viewProjection(relativeViewProjection)
        , frustum(Frustum::FromViewProjection(relativeViewProjection))
    {
    }

    glm::vec3 Relative(const glm::ivec3& worldPosition) const
    {
        const glm::vec3 cell = glm::floor(cameraPosition);
        return glm::vec3(worldPosition - glm::ivec3(cell)) - (cameraPosition - cell);
    }

    glm::vec3 Relative(const glm::vec3& worldPosition) const
    {
        return worldPosition - cameraPosition;
    }
};

}
