/*
 *  SimpleRenderEngine (https://github.com/mortennobel/SimpleRenderEngine)
 *
 *  Created by Morten Nobel-Jørgensen ( http://www.nobel-joergensen.com/ )
 *  License: MIT
 */

#pragma once

#include <array>
#include <memory>
#include "glm/glm.hpp"
#include "sre/Log.hpp"
#include "sre/impl/Export.hpp"

namespace sre {
    /**
     * The camera contains two important properties:
     * - view transform matrix: Contains information about location and orientation of the camera. This matrix will
     * transform geometry from world space to eye space.
     * - projection transform matrix: Contains information about the projection the camera uses (roughly equivalent to
     * which lens it uses). Generally this can either be perspective projection (with a field of view) or a orthographic
     * projection (without any perspective).
     *
     * The camera also includes information about the viewport, which defines which part of the window is used for
     * rendering (default settings is the full window (0,0) to (1,1))
     *
     * The default camera is positioned at (0,0,0) and looking down the negative z-axis. Everything inside the volume
     * between -1 to 1 is viewed.
     *
     * The coordinate system used is right-handed with y-pointing upwards.
     */
    class DllExport Camera {
    public:

        Camera();                                               // Set camera at (0,0,0) looking down the negative
                                                                // z-axis using orthographic viewing volume between -1 to 1

        void lookAt(glm::vec3 eye, glm::vec3 at, glm::vec3 up); // set position of camera in world space (view transform) using
                                                                // eye position of the camera
                                                                // at position that the camera looks at (must be different from pos)
                                                                // up the up axis (used for rotating camera around z-axis). Must not be parallel with view direction (at - pos).

        void setPositionAndRotation(glm::vec3 position, glm::vec3 rotationEulersDegrees);       // Set the camera view transform using worldspace position and rotation is degrees

        glm::vec3 getPosition();                                                                // Return the camera position (computed from the view transform)
        glm::vec3 getRotationEuler();                                                           // Return the camera rotation (from looking down negative z-axis) (computed from the view transform)

        std::array<glm::vec3,2> screenPointToRay(glm::vec2 position);                           // Returns a ray going from camera through a screen point.
                                                                                                // Resulting ray (position, direction) is in world space, starting on the near plane of the camera and going through position's (x,y) pixel coordinates on the screen.
                                                                                                // Screenspace is defined in pixels. The bottom-left of the screen is (0,0); the right-top is (pixelWidth,pixelHeight).
                                                                                                // Remember to invert y-axis when using windows coordinates pos.y = Renderer::instance->getWindowSize().y - pos.y;

        void setPerspectiveProjection(float fieldOfViewY, float nearPlane, float farPlane);      // set the projectionTransform to perspective projection
                                                                                                 // fieldOfViewY field of view in degrees
                                                                                                 // nearPlane near clipping plane, defines how close an object can be to the camera before clipping
                                                                                                 // farPlane far clipping plane, defines how far an object can be to the camera before clipping

        void setOrthographicProjection(float orthographicSize, float nearPlane, float farPlane); // set the projectionTransform to orthographic parallel viewing volume.
                                                                                                 // orthographicSize half the height of the view volume (the width is computed using the viewport size)
                                                                                                 // nearPlane near clipping plane, defines how close an object can be to the camera before clipping
                                                                                                 // farPlane far clipping plane, defines how far an object can be to the camera before clipping

        void setWindowCoordinates();                             // set orthographic transform and view, where the origin is located in the lower left corner
                                                                 // z depth is between -1 and 1
                                                                 // width the width of the window
                                                                 // height the height of the window

        void setViewTransform(const glm::mat4 &viewTransform);   // Set the view transform. Used to position the virtual camera position and orientation.
                                                                 // This is commonly set using lookAt


        void setProjectionTransform(const glm::mat4 &projectionTransform);                       // Set a custom projection transform. Defines the view volume and how it is projected to the screen.
                                                                                                 // This is commonly set using setPerspectiveProjection(), setOrthographicProjection() or setWindowCoordinates()


        glm::mat4 getViewTransform();                            // Get the view transform. The matrix transformation
                                                                 // contain the orientation and position of the virtual
                                                                 // camera.

        glm::mat4 getProjectionTransform(glm::uvec2 viewportSize);// Get the projection transform - used for rendering

        glm::mat4 getInfiniteProjectionTransform(glm::uvec2 viewportSize);// Get the projection transform - used for skybox rendering


        void setViewport(glm::vec2 offset = glm::vec2{0,0}, glm::vec2 size = glm::vec2{1,1});    // defines which part of the window is used for
                                                                                                 // rendering (default settings is the full window).
                                                                                                 // x the x coordinate of the viewport (default 0)
                                                                                                 // y the y coordinate of the viewport (default 0)
                                                                                                 // width the width of the viewport (default window width)
                                                                                                 // height the height of the viewport (default window height)
        bool isChanged();                                        // Returns true if the camera has been changed since the last time the changed flag was reset
        void resetChangedFlag();                                 // Reset the changed flag to false
    protected:
        void setChangedFlag();
        enum class ProjectionType {
            Perspective,
            Orthographic,
            OrthographicWindow,
            Custom
        };
        ProjectionType projectionType = ProjectionType::Orthographic;
    private:
        bool m_isChanged = false;
        union {
            struct {
                float fieldOfViewY;
                float nearPlane;
                float farPlane;
            } perspective;
            struct {
                float orthographicSize;
                float nearPlane;
                float farPlane;
            } orthographic;
            float customProjectionMatrix[16];

        } projectionValue;

        glm::mat4 viewTransform;

        glm::vec2 viewportOffset = glm::vec2{0,0};
        glm::vec2 viewportSize = glm::vec2{1,1};

        friend class RenderPass;
        friend class Inspector;

    };

    // Camera base class that can be used to create custom cameras.
    // See FlightCamera and FPS_Camera classes for examples.

    template <typename T>
    class CustomCameraBuilder;

    template <typename T>
    class DllExport CustomCamera : public Camera {
    // Note that the following functions from the Camera base class:
    //      lookAt, setPositionAndRotation, setPerspectiveProjection,
    //      setProjectionTransform, setWindowCoordinates, setViewTransform,
    //      setProjectionTransform
    // will disrupt the cameras derived from CustomCamera. Rather than
    // preventing their use (for unanticpated, legitimate purposes), we
    // trust that derived classes from CustomCamera will use them
    // appropriately in conjunction with the CustomCamera class.
    public:
        virtual ~CustomCamera() = default;
        // Get and set speed that camaera moves (in world units/sec)
        void setSpeed(float speedIn);
        float getSpeed();
        float getRotationSpeed();
        void setRotationSpeed(float rotationSpeed);
        // Set generic camera properties
        void setPosition(glm::vec3 positionIn);
        void setNearPlane(float nearPlaneIn);
        void setFarPlane(float farPlaneIn);
        // Set OrthographicProjection camera properties
        void setWorldHalfHeight(float worldHalfHeightIn);
        // Set projection camera properties
        void setFieldOfView(float fieldOfViewIn);
        void setMaxFieldOfView(float maxFieldOfViewIn);
        // Move the position of the camera by a vector delta
        void move(glm::vec3 deltaVector);
        // 'Zoom' camera by changing field-of-view or world-half-height
        virtual void zoom(float zoomIncrement); 
    protected:
        friend class CustomCameraBuilder<T>;
        CustomCamera();         // Need to set either worldHalfHeight (to
                                // initialize Orthographic view) or fieldOfView
                                // (to initialize Perspective view)
        void init();            // Initialize the camera with existing values
        glm::vec3 position;     // Position of the camera in world space
        glm::vec3 direction;    // Unit vector pointing from camera to target
        glm::vec3 up;           // Unit vector perpendicular to direction
        glm::vec3 right;        // Unit vector perpendicular to direction & up
        // Near and far plane for projection
        float nearPlane;
        float farPlane;
        // Half of height of window in world coords
        // Used for Orhtographic camera
        float worldHalfHeight;
        // Camera 'Field of View' in degrees ('warping' appears > 45.0)
        // Used for Perspective camera
        float fieldOfView;
        float maxFieldOfView;
        // Camera speed is distance (in world-space units) covered per second
        float speed;
        // Camera rotation speed is in degrees/sec
        float rotationSpeed;
    };

    template <typename T>
    class DllExport CustomCameraBuilder {
    public:
        virtual ~CustomCameraBuilder() = default;
        T& withPosition(glm::vec3 position);
        T& withDirection(glm::vec3 direction);
        T& withUpDirection(glm::vec3 upDirection);
        T& withSpeed(float speed);
        T& withRotationSpeed(float rotationSpeed);
        T& withFieldOfView(float fieldOfView);
        T& withMaxFieldOfView(float maxFieldOfView);
        T& withNearPlane(float nearPlane);
        T& withFarPlane(float farPlane);
    protected:
        friend class CustomCamera<T>;
        CustomCameraBuilder() = default;
        std::shared_ptr<CustomCamera<T>> camera = nullptr;
    };
    
    class FlightCameraBuilder;

    // Custom basic flight camera. Currently does not visualize a cockpit
    class DllExport FlightCamera : public CustomCamera<FlightCameraBuilder> {
    public:
        virtual ~FlightCamera() = default;
        // Create FlightCamera with 'builder' pattern in CustomCamera base class
        static FlightCameraBuilder create();
        // Move the camera by disance in the direction camera is pointing
        virtual void move(float distance);
        using CustomCamera<FlightCameraBuilder>::move;
        // Change direction camera is pointing according to pitch and yaw
        virtual void pitchAndYaw(float pitchIncrement, float yawIncrement);
        virtual void roll(float rollIncrement);
    protected:
        friend class FlightCameraBuilder;
        struct MakeSharedEnabler; // Enable make_shared to use protected ctor
        FlightCamera() = default;
    };

    struct FlightCamera::MakeSharedEnabler : public FlightCamera {
    // std::make_shared requires a public constructor
    public:
        MakeSharedEnabler() : FlightCamera() {}
    };

    class DllExport FlightCameraBuilder
                    : public CustomCameraBuilder<FlightCameraBuilder> {
    public:
        ~FlightCameraBuilder() = default;
        // Inherits all the CustomCameraBuilder<T>::with... methods
        std::shared_ptr<FlightCamera> build();
    protected:
        friend class FlightCamera;
        FlightCameraBuilder();
    };

    class FPS_CameraBuilder;

    // Custom basic First-Person Surveyor (a.k.a. Minecraft-like) camera
    class DllExport FPS_Camera : public CustomCamera<FPS_CameraBuilder> {
    public:
        virtual ~FPS_Camera() = default;
        // Create FPS_Camera with 'builder' pattern in CustomCamera base class
        static FPS_CameraBuilder create();
        enum class Direction {Forward, Backward, Left, Right, Up, Down};
        // Move the camera horizontally (perpendicular to worldUp Direction)
        virtual void move(float distance, Direction direction);
        using CustomCamera<FPS_CameraBuilder>::move;
        // Change direction camera is pointing according to pitch and yaw
        virtual void pitchAndYaw(float pitchIncrement, float yawIncrement);
    protected:
        friend class FPS_CameraBuilder;
        struct MakeSharedEnabler; // Enable make_shared to use protected ctor
        FPS_Camera() = default;
        void init();
        // "World up" direction vector in world coordinates (needed to project
        // the direction vector onto the horizontal plane for an FPS camera)
        glm::vec3 worldUp; 
        // Vector pointing in the direction the camera will move (constrained
        // to horizontal plane for an FPS camera - projection of up onto plane)
        glm::vec3 forward;
        // Length of the forward vector before it is normalized
        float forwardLen;
    };

    struct FPS_Camera::MakeSharedEnabler : public FPS_Camera {
    // std::make_shared requires a public constructor
    public:
        MakeSharedEnabler() : FPS_Camera() {}
    };

    class DllExport FPS_CameraBuilder
                    : public CustomCameraBuilder<FPS_CameraBuilder> {
    public:
        ~FPS_CameraBuilder() = default;
        FPS_CameraBuilder& withWorldUpDirection(glm::vec3 worldUpDirection);
        FPS_CameraBuilder& withWorldHalfHeight(float worldHalfHeight);
        std::shared_ptr<FPS_Camera> build();
    private:
        friend class FPS_Camera;
        FPS_CameraBuilder();
    };

// CustomCamera Class Method Implementations ==================================

template<typename T>
CustomCamera<T>::CustomCamera() : Camera(),
                             position {0.0, 0.0, 0.0},
                             direction {0.0, 0.0, -1.0},
                             up {0.0, 1.0, 0.0},
                             speed {1.0f},
                             rotationSpeed {1.0f},
                             worldHalfHeight {0.0f},
                             fieldOfView{0.0f},
                             nearPlane {0.1f},
                             farPlane {100.0f} {
    maxFieldOfView = fieldOfView;
    setChangedFlag();
}   

template<typename T>
void CustomCamera<T>::init() {
    direction = glm::normalize(direction);
    up = glm::normalize(up);
    right = glm::cross(direction, up);
    if (worldHalfHeight > 0.0f && fieldOfView > 0.0f)
    {
        LOG_ERROR("Should not set both worldHalfHeight (for " 
        "Orthographic Projection) and fieldOfView (for Perspective " 
        "Projection). Choosing Perspective");
        worldHalfHeight = 0.0f;
    }   
    if (worldHalfHeight > 0.0f)
        setOrthographicProjection(worldHalfHeight, nearPlane, farPlane);
    else if (fieldOfView > 0.0f)
        setPerspectiveProjection(fieldOfView, nearPlane, farPlane);
    else
    {   // Default to perspective projection
        fieldOfView = 45.0f;
        setPerspectiveProjection(fieldOfView, nearPlane, farPlane);
    }
    lookAt(position, position + direction, up);
    setChangedFlag();
}

template<typename T> void
CustomCamera<T>::setSpeed(float speedIn) {
    speed = speedIn;
    setChangedFlag();
}

template<typename T> float
CustomCamera<T>::getSpeed() {
    return speed;
}

template<typename T> void
CustomCamera<T>::setRotationSpeed(float rotationSpeedIn) {
    rotationSpeed = rotationSpeedIn;
    setChangedFlag();
}

template<typename T> float
CustomCamera<T>::getRotationSpeed() {
    return rotationSpeed;
}

template<typename T> void
CustomCamera<T>::setWorldHalfHeight(float worldHalfHeightIn) {
    worldHalfHeight = worldHalfHeightIn;
    setOrthographicProjection(worldHalfHeight, nearPlane, farPlane);
    setChangedFlag();
}

template<typename T> void
CustomCamera<T>::setFieldOfView(float fieldOfViewIn) {
    fieldOfView = fieldOfViewIn;
    if (fieldOfView > 0.0)
        setPerspectiveProjection(fieldOfView, nearPlane, farPlane);
    setChangedFlag();
}

template<typename T> void
CustomCamera<T>::setMaxFieldOfView(float maxFieldOfViewIn) {
    maxFieldOfView = maxFieldOfViewIn;
    if (fieldOfView > 0.0)
    {
        fieldOfView = glm::min(fieldOfView, maxFieldOfView);
        setPerspectiveProjection(fieldOfView, nearPlane, farPlane);
    }
    setChangedFlag();
}

template<typename T> void
CustomCamera<T>::setPosition(glm::vec3 positionIn) { 
    position = positionIn;
    // Set the camera view transform
    lookAt(position, position + direction, up);
    setChangedFlag();
}

template<typename T> void
CustomCamera<T>::setNearPlane(float nearPlaneIn) {
    nearPlane = nearPlaneIn;
    if (fieldOfView > 0.0)
        setPerspectiveProjection(fieldOfView, nearPlane, farPlane);
    setChangedFlag();
}

template<typename T> void
CustomCamera<T>::setFarPlane(float farPlaneIn) {
    farPlane = farPlaneIn;
    if (fieldOfView > 0.0)
        setPerspectiveProjection(fieldOfView, nearPlane, farPlane);
    setChangedFlag();
}

template<typename T> void
CustomCamera<T>::move(glm::vec3 deltaVector) {
    position += deltaVector;
    // Set the camera view transform
    lookAt(position, position + direction, up);
    setChangedFlag();
}

template<typename T> void
CustomCamera<T>::zoom(float zoomIncrement) {
    if (fieldOfView > 0.0)
    {   // For Perpective Projection camera
        fieldOfView -= zoomIncrement; // Decreasing the FOV increases "zoom"
        fieldOfView = glm::min(fieldOfView, maxFieldOfView);
        fieldOfView = glm::max(fieldOfView, 1.0f);
        setPerspectiveProjection(fieldOfView, nearPlane, farPlane);
    }
    else if (worldHalfHeight > 0.0)
    {   // For Orthographic Projection camera
        worldHalfHeight *= (1.0f + zoomIncrement);
        setOrthographicProjection(worldHalfHeight, nearPlane, farPlane);
    }
    setChangedFlag();
}

// CustomCameraBuilder Class Method Implementations ============================

template<typename T> T&
CustomCameraBuilder<T>::withPosition(glm::vec3 position) {
    camera->position = position;
    return dynamic_cast<T&>(*this);
}

template<typename T> T&
CustomCameraBuilder<T>::withDirection(glm::vec3 direction) {
    camera->direction = direction;
    return dynamic_cast<T&>(*this);
}

template<typename T> T&
CustomCameraBuilder<T>::withUpDirection(glm::vec3 upDirection) {
    camera->up = upDirection;
    return dynamic_cast<T&>(*this);
}

template<typename T> T&
CustomCameraBuilder<T>::withSpeed(float speed) {
    camera->speed = speed;
    return dynamic_cast<T&>(*this);
}

template<typename T> T&
CustomCameraBuilder<T>::withRotationSpeed(float rotationSpeed) {
    camera->rotationSpeed = rotationSpeed;
    return dynamic_cast<T&>(*this);
}

template<typename T> T&
CustomCameraBuilder<T>::withFieldOfView(float fieldOfView) {
    camera->fieldOfView = fieldOfView;
    return dynamic_cast<T&>(*this);
}

template<typename T> T&
CustomCameraBuilder<T>::withMaxFieldOfView(float maxFieldOfView) {
    camera->maxFieldOfView = maxFieldOfView;
    return dynamic_cast<T&>(*this);
}

template<typename T> T&
CustomCameraBuilder<T>::withNearPlane(float nearPlane) {
    camera->nearPlane = nearPlane;
    return dynamic_cast<T&>(*this);
}

template<typename T> T&
CustomCameraBuilder<T>::withFarPlane(float farPlane) {
    camera->farPlane = farPlane;    
    return dynamic_cast<T&>(*this);
}

}

