#include <iostream>
#include <vector>
#include <fstream>

#include "sre/Renderer.hpp"
#include "sre/SDLRenderer.hpp"
#include <sre/Skybox.hpp>
#include <sre/ModelImporter.hpp>

using namespace sre;
using namespace glm;

class FPS_Camera_Demo {
public:
FPS_Camera_Demo() {
	// Define and initialize Graphics renderer (needs to be done first)
    SDLRenderer renderer;
    renderer.init();
	// Assign SDLRenderer 'callback' functions to functions implemented below
    // Alternatively, Lambda expressions can be used, as in other examples
    renderer.frameUpdate
        = std::bind(&FPS_Camera_Demo::frameUpdate, this, std::placeholders::_1);
    renderer.frameRender
        = std::bind(&FPS_Camera_Demo::frameRender, this);
    renderer.mouseEvent
        = std::bind(&FPS_Camera_Demo::mouseEvent, this, std::placeholders::_1);
    renderer.keyEvent
        = std::bind(&FPS_Camera_Demo::keyEvent, this, std::placeholders::_1);

	// Create camera
	glm::vec3 position {0.0f, 0.0f, 50.0f * worldUnit};
	glm::vec3 direction {0.0f, 0.0f, -1.0f};
	glm::vec3 worldUp {0.0f, 1.0f, 0.0f};
	float speed = 2.0f * worldUnit; // 2 worldUnits / second
	float rotationSpeed = 5.0f; // 5 degrees / second
	float fieldOfView= 45.0f;
//  The code below illustrates two alternative options to using the "Init-Builder"
//  paradigm used to initialize the camera
//	camera->init(position,direction,worldUp, speed,rotationSpeed,fieldOfView);
//	camera->init({0.0f, 0.0f, 10.0f * worldUnit}, // position
//				{0.0f, 0.0f, -1.0f}, // direction
//				{0.0f, 1.0f,  0.0f}, // worldUp
//				2.0f * worldUnit, 5.0f, // speed, rotationSpeed
//				45.0f)  // fieldOfView
	camera = FPS_Camera::create()
				.withPosition(position)
				.withDirection(direction)
				.withUpDirection(worldUp)
				.withWorldUpDirection(worldUp)
				.withSpeed(speed)
				.withRotationSpeed(rotationSpeed)
				.withFieldOfView(fieldOfView)
				.withFarPlane(150.0f)
				.build();

	// Create lighting
    worldLights.setAmbientLight({0.05f, 0.05f, 0.05f});
	Light sun = Light::create()
				.withDirectionalLight({1.0f, 1.0f, 1.0f})
				.withColor({1.0f, 1.0f, 1.0f})
				.build();
    worldLights.addLight(sun);

	// Create the sky (with a horizon, called the 'Skybox')
    skybox = Skybox::create();

	// Create a grid (wireframe) plane at top of domain
    auto gridPlaneTopMaterial = Shader::getUnlit()->createMaterial();
	gridPlaneTopMaterial->setColor({0.0f, 0.0f, 0.0f, 1.0f});
    gridPlaneTop = Mesh::create()
					.withWirePlane(30) // 30 intervals 
					.withLocation({0.0f, 20.0f * worldUnit, 0.0f})
					.withScaling(75.0f * worldUnit)
					.withMaterial(gridPlaneTopMaterial)
					.build();

	// Create a grid (wireframe) plane at bottom of the domain
    auto gridPlaneBottomMaterial = Shader::getUnlit()->createMaterial();
	gridPlaneBottomMaterial->setColor({1.0f, 1.0f, 1.0f, 1.0f});
    gridPlaneBottom = Mesh::create()
					.withWirePlane(30) // 30 intervals 
					.withLocation({0.0f, -20.0f * worldUnit, 0.0f})
					.withScaling(75.0f * worldUnit)
					.withMaterial(gridPlaneBottomMaterial)
					.build();

	// Create torus
	auto torusMaterial = Shader::getStandardPBR()->createMaterial();
	torusMaterial->setColor({1.0f,1.0f,1.0f,1.0f});
    torusMaterial->setMetallicRoughness({0.5f, 0.5f});
	int segmentsC = 48;
	int segmentsA = 48;
    torus = Mesh::create()
					.withTorus(segmentsC, segmentsA)
					.withLocation({0.0f, 0.0f, 0.0f})
					.withRotation({45.0f, 45.0f, 0.0f})
					.withScaling({3.0f * worldUnit, 2.0f * worldUnit,
													1.0f * worldUnit})
					.withMaterial(torusMaterial)
					.build();

	// Create sphere
    auto sphereMaterial = Shader::getStandardPBR()->createMaterial();
    sphereMaterial->setColor({0.0f, 1.0f, 0.0f, 1.0f});
    sphereMaterial->setMetallicRoughness({0.5f, 0.5f});
	int stacks = 32;
	int slices = 64;
    sphere = Mesh::create()
					.withSphere(stacks, slices)
					.withLocation({-20.0f * worldUnit, 0.0f, 0.0f})
					.withScaling(worldUnit)
					.withMaterial(sphereMaterial)
					.build();

	// Create the monkey object "Suzanne"
    auto SuzanneMaterial = Shader::getStandardPBR()->createMaterial();
    SuzanneMaterial->setColor({1.0f, 0.7f, 0.2f, 1.0f});
    SuzanneMaterial->setMetallicRoughness({0.5f, 0.5f});
	Suzanne = sre::ModelImporter::importObj("examples_data/", "suzanne.obj");
	Suzanne->setLocation({20.0f * worldUnit, 0.0f, 0.0f});
	Suzanne->setRotation({0.0f, -45.0f, 0.0f});
	Suzanne->setScaling(worldUnit);
	Suzanne->setMaterial(SuzanneMaterial);

	// Capture the mouse (mouse won't be visible)
	//SDL_SetRelativeMouseMode(SDL_TRUE);
	//SDL_CaptureMouse(SDL_TRUE);

	// Start processing mouse and keyboard events (continue until user quits)
    renderer.startEventLoop();
}

// Callback functions for the FPS_Camera_Demo class ============================

// Update the rendering frame (move the sphere)
void frameUpdate(float deltaTime) {
	elapsedTime++;
	glm::vec3 sphereLocation = sphere->getLocation();	
	sphereLocation.z += cos(elapsedTime/50.0f)/7.0f;
	sphere->setLocation(sphereLocation);
};

// Render (draw) the updated frame
void frameRender() {
	// Create render pass, initialize with global variables
	auto renderPass = RenderPass::create()
   	     .withCamera(*camera)
   	     .withWorldLights(&worldLights)
   	     .withSkybox(skybox)
   	     .withName("Frame")
   	     .build();
	// Draw objects
	gridPlaneTop->draw(renderPass);
	gridPlaneBottom->draw(renderPass);
	torus->draw(renderPass);
	sphere->draw(renderPass);
	Suzanne->draw(renderPass);
};

// Process keyboard events
void keyEvent(SDL_Event& event) {
	auto key = event.key.keysym.sym;
	// Check for key presses and take appropriate action
	if (event.type == SDL_KEYDOWN) {
		if (key == SDLK_EQUALS || key == SDLK_MINUS) {
			float zoomIncrement = 5.0;
			if (key == SDLK_EQUALS) {
				camera->zoom(zoomIncrement);
			} else if (key == SDLK_MINUS) {
				camera->zoom(-zoomIncrement);
			}
		}
		if (key == SDLK_w || key == SDLK_k || key == SDLK_UP ||
			key == SDLK_s || key == SDLK_j || key == SDLK_DOWN ||
			key == SDLK_a || key == SDLK_h || key == SDLK_LEFT ||
			key == SDLK_d || key == SDLK_l || key == SDLK_RIGHT ||
			key == SDLK_LCTRL || key == SDLK_SPACE || key == SDLK_LSHIFT ||
			key == SDLK_z) {
			// Calculate the distance the camera should move
			// Assume user presses a key in 1/5 of a second (5 presses per sec)
			// Distance traveled in 1/5 of a second = speed (unit/s) * time (s)
			float distance = camera->getSpeed() * 1.0/5.0;
			if (key == SDLK_w || key == SDLK_k || key == SDLK_UP) {
				// Move camera forward in horizontal plane towards target
				camera->move(distance, FPS_Camera::Direction::Forward);
			} else if (key == SDLK_s || key == SDLK_j || key == SDLK_DOWN) {
				// Move camera backward in horizontal plane away from target
				camera->move(distance, FPS_Camera::Direction::Backward);
			} else if (key == SDLK_a || key == SDLK_h || key == SDLK_LEFT) {
				// Move camera (strafe) left
				camera->move(distance, FPS_Camera::Direction::Left);
			} else if (key == SDLK_d || key == SDLK_l || key == SDLK_RIGHT) {
				// Move camera (strafe) right
				camera->move(distance, FPS_Camera::Direction::Right);
			} else if (key == SDLK_SPACE) {		// Minecraft uses LCTRL || SPACE
				// Move camera up vertically 
				camera->move(distance, FPS_Camera::Direction::Up);
			} else if (key == SDLK_z) {   		// Minecraft uses LSHIFT
				// Move camera down vertically	// Max uses  LCTRL
				camera->move(distance, FPS_Camera::Direction::Down);
			}
		}
	}
}

// Process mouse events
void mouseEvent(SDL_Event& event) {
	// SDL_GetMouseState(&xpos, &ypos); // Use to set the mouse position?
    if (event.type == SDL_MOUSEBUTTONDOWN) {
		lastMouse_x = event.button.x;	
		lastMouse_y = event.button.y;	
		mouseDown = true;
	} else if (event.type == SDL_MOUSEBUTTONUP) {
		mouseDown = false;
	} else if (event.type == SDL_MOUSEMOTION && mouseDown) {
		// Mouse movement is degrees per pixel moved
        float degreesPerPixel = 0.02f * camera->getRotationSpeed();
		float yaw = (event.motion.x - lastMouse_x) * degreesPerPixel;
		float pitch = (lastMouse_y - event.motion.y) * degreesPerPixel;
		lastMouse_x = event.motion.x;
		lastMouse_y = event.motion.y;
		camera->pitchAndYaw(pitch, yaw);
    } else if (event.type == SDL_MOUSEWHEEL){ // Works for two-finger touch
		float zoomPerClick = 0.5f;
		float zoomIncrement = event.wheel.y * zoomPerClick;
		camera->zoom(zoomIncrement);
	}
}

// Private Member Variables of the FPS_Camera_Demo class =======================
private:

    // Environment
    std::shared_ptr<FPS_Camera> camera;
    WorldLights worldLights;
    std::shared_ptr<Skybox> skybox;
    float elapsedTime = 0.0f;
    float worldUnit = 1.0; // Used as a unit of measure to scale all objects
    
    // Objects
    std::shared_ptr<Mesh> gridPlaneTop;
    std::shared_ptr<Mesh> gridPlaneBottom;
    std::shared_ptr<Mesh> torus;
    std::shared_ptr<Mesh> sphere;
    std::shared_ptr<Mesh> Suzanne; // Monkey object
    
    // Mouse callback state
    bool mouseDown = false;
    int lastMouse_x = 0;
    int lastMouse_y = 0;
};

// Main function ===============================================================

int main() {
    FPS_Camera_Demo fpsDemo;
    return 0;
}
