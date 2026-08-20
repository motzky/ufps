#pragma once

#include "core/camera.h"
#include "core/camera_manager.h"
#include "core/service_locator.h"

namespace ufps
{
    class Actor
    {
    public:
        constexpr Actor(CameraHandle camera);
        virtual ~Actor() = default;

        Actor(const Actor &) = delete;
        auto operator=(const Actor &) -> Actor & = delete;
        Actor(Actor &&) = default;
        auto operator=(Actor &&) -> Actor & = default;

        virtual auto update() -> void = 0;

        constexpr auto &camera(this auto &&self);

    protected:
        CameraHandle _camera;
    };

    constexpr Actor::Actor(CameraHandle camera)
        : _camera{camera}
    {
    }

    constexpr auto &Actor::camera(this auto &&self)
    {
        return self._camera;
    }
}