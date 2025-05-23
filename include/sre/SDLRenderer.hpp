/*
 *  SimpleRenderEngine (https://github.com/mortennobel/SimpleRenderEngine)
 *
 *  Created by Morten Nobel-Jørgensen ( http://www.nobel-joergensen.com/ )
 *  License: MIT
 */

#pragma once

#ifdef _SDL_H
#error SDL should not be included before SDLRenderer
#endif

#define SDL_MAIN_HANDLED

#include "SDL.h"


#include <functional>
#include <string>
#include <sstream>
#include <filesystem>
#include <chrono>
#include "sre/Renderer.hpp"



namespace sre {

// forward declaration
class Renderer;

// Simplifies SDL applications by abstracting away boilerplate code.
//
// SDLRenderer is a pure helper-class, and no other class in the SimpleRenderEngine depends on it.
//
// The class will create a window with a graphics context in the `init()` member function.
// The `startEventLoop()` will start the event loop, which polls the event queue in the
// beginning of each frame (and providing callbacks to `keyEvent` and `mouseEvent`), followed by a `frameUpdate(float)`
// and a `frameRender()`.

typedef std::chrono::high_resolution_clock Clock;

class DllExport SDLRenderer {
public:
    class InitBuilder {
    public:
        ~InitBuilder();
        InitBuilder& withSdlInitFlags(uint32_t sdlInitFlag);            // Set SDL Init flags (See: https://wiki.libsdl.org/SDL_Init )
        InitBuilder& withSdlWindowFlags(uint32_t sdlWindowFlags);       // Set SDL Window flags (See: https://wiki.libsdl.org/SDL_WindowFlags )
        InitBuilder& withDpiAwareness(bool isDpiAware);                 // Set SDL to be "DPI" (dots per inch) aware (SDL window rescales for high-dpi screens)
        InitBuilder& withVSync(bool vsync);
        InitBuilder& withGLVersion(int majorVersion, int minorVersion);
        InitBuilder& withMaxSceneLights(int maxSceneLights);            // Set max amount of concurrent lights
        InitBuilder& withMinimalRendering(bool minimalRendering);       // Minimize rendering for graphics based on the interaction of the user (stop
                                                                        // rendering when the user stops interacting). This conserves considerable power,
                                                                        // which is especially important for laptop battery life.
                                                                        // If you are using this option with ImGui, the blinking cursor is turned off
                                                                        // (if this was not done, the cursor wouuld stop blinking whenever rendering is paused,
                                                                        // leading to a "flaky" experience for the user.
        void build();
    private:
        explicit InitBuilder(SDLRenderer* sdlRenderer);
        SDLRenderer* sdlRenderer;
        uint32_t sdlInitFlag = SDL_INIT_EVERYTHING;
        uint32_t sdlWindowFlags = SDL_WINDOW_ALLOW_HIGHDPI  | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
        bool isDpiAware = false;
        bool vsync = true;
        int glMajorVersion = 3;
        int glMinorVersion = 3;
        int maxSceneLights = 4;
        bool minimalRendering = false;
        friend class SDLRenderer;
    };

    SDLRenderer();
    virtual ~SDLRenderer();

    // event handlers (assigned empty default handlers)
    std::function<void(float deltaTimeSec)> frameUpdate;        // Callback every frame with time since last callback in seconds
    std::function<void()> frameRender;                          // Callback to render events - called after frameUpdate. The `Renderer::swapFrame()` is
                                                                // automatically invoked after the callback.
    std::function<void()> windowMaximized;                      // Callback when the main window has been maximized by the user
    std::function<void()> windowRestored;                       // Callback when the main window has been restored by the user
    std::function<void()> windowSizeChanged;                    // Callback when the main window size has been changed by the user
    std::function<void()> stopProgram;                          // Callback to stop the program -- called upon "SDL_QUIT". This gives the program an opportunity
                                                                // to perform and orderly shutdown before calling "stopEventLoop()" to end the program.
    std::function<void(std::ostringstream& userMessage)>
                                               handleException; // Callback to perform actions before termination -- append to message stream if desired.
    std::function<void()> userClickedOutsideModalTwice;         // Callback when user clicks mouse outside a modal dialog twice (indicates user unaware of modal)
    std::function<void(SDL_Event& e)> keyEvent;                 // Callback of `SDL_KEYDOWN` and `SDL_KEYUP`.
    std::function<void(SDL_Event& e)> mouseEvent;               // Callback of `SDL_MOUSEMOTION`, `SDL_MOUSEBUTTONDOWN`, `SDL_MOUSEBUTTONUP`, `SDL_MOUSEWHEEL`.
    std::function<void(SDL_Event& e)> controllerEvent;          // Callback of `SDL_CONTROLLERAXISMOTION`, `SDL_CONTROLLERBUTTONDOWN`, `SDL_CONTROLLERBUTTONUP`,
                                                                // `SDL_CONTROLLERDEVICEADDED`, `SDL_CONTROLLERDEVICEREMOVED` and `SDL_CONTROLLERDEVICEREMAPPED`.
    std::function<void(SDL_Event& e)> joystickEvent;            // Callback of `SDL_JOYAXISMOTION`, `SDL_JOYBALLMOTION`, `SDL_JOYHATMOTION`, `SDL_JOYBUTTONDOWN`,
                                                                // `SDL_JOYBUTTONUP`, `SDL_JOYDEVICEADDED`, `SDL_JOYDEVICEREMOVED`.
    std::function<void(SDL_Event& e)> touchEvent;               // Callback of `SDL_FINGERDOWN`, `SDL_FINGERUP`, `SDL_FINGERMOTION`.
    std::function<void(SDL_Event& e)> otherEvent;               // Invoked if unhandled SDL event

    InitBuilder init();                                         // Create the window and the graphics context (instantiates the sre::Renderer). Note that most
                                                                // other sre classes requires the graphics content to be created before they can be used (e.g.
                                                                // a Shader cannot be created before `init()`).
                                                                // The initialization happens on InitBuilder::build or InitBuilder::~InitBuilder()

    void setWindowTitle(std::string title);
    void setWindowIcon(std::shared_ptr<Texture> tex);           // Set application icon
    void setWindowPosition(glm::ivec2 size);                    // Set application position (default is centered)
    void setWindowSize(glm::ivec2 size);
    glm::ivec2 getWindowSize();                                 // Return the current size of the window (via embedded renderer object - may be scaled by OS)
    glm::ivec2 getWindowSizeInPixels();                         // Return the current size of the window (via the embedded renderer object - actual pixel size)
    glm::ivec2 getDrawableSize();                               // Return the current drawable size of the window (via the embedded renderer object)
    float getDisplayScale();

    void setFullscreen(bool enabled = true);                    // Toggle fullscreen mode (default mode is windowed). Not supported in Emscripten
    bool isFullscreen();

    void setMouseCursorVisible(bool enabled = true);            // Show/hide mouse cursor. Not supported in Emscripten
    bool isMouseCursorVisible();                                // GUI should not be rendered when mouse cursor is not visible (this would force the mouse
                                                                // cursor to appear again)

    bool setMouseCursorLocked(bool enabled = true);             // Lock the mouse cursor, such that mouse cursor motion is detected, (while position remains
                                                                // fixed). Not supported in Emscripten
    bool isMouseCursorLocked();                                 // Locking the mouse cursor automatically hides the mouse cursor
    SDL_Cursor* createMouseCursorFromXPM(const char* cursorImage[]); // Create an SDL mouse cursor from a character array representing an XPM-formated cursor
    void setMouseCursor(SDL_Cursor* cursorIn);                  // Change the mouse cursor to the cusorIn (which can be created with createMouseCursorFromXPM)
    void restoreMouseCursor();                                  // Restore the cursor to the last setMouseCursor(cursorIn) call
    void setArrowMouseCursor();                                 // Set the mouse cursor to the default "arrow"
    void setWaitMouseCursor();                                  // Set the mouse cursor to "wait" (usually an hourglass or a timer image)
    void setResizeAllMouseCursor();                             // Set the mouse cursor to "resize all" (usually a n-e-s-w or a hand image)

    void startEventLoop();                                      // Start the event loop. Note that this member function in usually blocking (until the
                                                                // `stopEventLoop()` has been  called). Using Emscripten the event loop is not blocking (but
                                                                // internally using a callback function), which means that when using Emscripten avoid
                                                                // allocating objects on the stack (see examples for a workaround).

    void stopEventLoop();                                       // The render loop will stop running when the frame is complete.

    void startEventSubLoop();                                   // Start a second event loop within the main event loop. Useful when mouse and keyboard events
                                                                // need to be captured deep within a time-consuming function. Note that there will be a
                                                                // discrepancy between the detaTime passed to the first call to frameUpdate(deltaTimeSec) in
                                                                // the event sub-loop and the last call in the main event loop.

    void stopEventSubLoop();                                    // The render sub-loop will stop running when the frame is complete.

    void getAndProcessEvents();                                 // Call this function to keep application responsive to OS (either Wayland or SDL will give a
                                                                // message that the app is not responding if this is not called during long calculations).
    bool processKeyPressedAndMouseDownEvents(                   // Call this function before a long calculation so that ImGui doesn't think that a key or 
                         std::string * errorMessage = nullptr); // mouse button is still down (this can cause unpredictable behavior depending on the key)
    void startTimerForKeepAppResponsive();                      // Start the timer for keeping the app responsive
    void keepAppResponsive();                                   // Process events and draw at set, timed intervals
    void drawFrame();                                           // Draw a single frame. This is useful when application graphics need to be updated from deep
                                                                // within a time-consuming function while not desiring user input (for example, a progress
                                                                // dialog).
    int getFrameNumber();                                       // Get the current drawing frame number. This is useful to label output (e.g. screen images).

    void startEventLoop(std::shared_ptr<VR> vr);                // Start event loop for VR

    SDL_Window *getSDLWindow();                                 // Get a pointer to SDL_Window

    static SDLRenderer* instance;                               // Singleton reference to the engine after initialization.

    glm::vec3 getLastFrameStats();                              // Returns delta time for last frame wrt event, update and render

    void SetMinimalRendering(bool minimalRendering);            // If SetMinimalRendering is passed "true", then SRE will minimize the number of rendering
                                                                // operations performed -- SRE will only render upon mouse or keyboard activity, or if the
                                                                // application notifies SRE that it has been updated through the SetAppUpdated method.

    void SetAppUpdated(bool appUpdated);                        // Let SRE know that the application has updated so that it will force rendering when
                                                                // using the flag MiniumalRendering. This will be set to "false" after the next render operation.

    // Event recording and playback interface
    void AutoRecordEvents() {m_autoRecordEvents = true;}        // Automatically record events to an archive for later playback (helpful for identifying issues)
    std::stringstream GetSettingsFromEventsFile(                // Find the json settings file embedded in the events file (a specific header format is required).
                            const std::string& eventsfileName); // If not found, return an empty stringstream.
    void SetJsonSettingsForEventRecording(                      // Set the json settings file (embedded in a single std:string) that will be incorporated into
                            const std::string& settings)        // the events file when written.
                                    {m_jsonSettings = settings;}
    bool setupEventRecorder(bool& recordingEvents,              // Setup the SDL event (e.g. keyboard, mouse, mouse motion, etc.) recording and playback
                            bool& playingEvents,                // functionality based on the parameters given. The function returns true if successful, false if
                            const std::string& recordEventsFile,// not. If there is an error, then the error message is returned in the errorMessage string, and
                            const bool& overWriteRecordingFile, // the recording and playing flags will be set to false. It is recommended to only record right
                            const std::string& playEventsFile,  // after the host application has started (e.g. start recording only based on a flag passed as an
                            std::string&  errorMessage);        // argument) because the starting state of an application is nearly impossible to characterize after
                                                                // a user has been operating the gui for even a short amount of time. 
    bool setupAndStartEventRecorder(bool& recordingEvents,      // This is the same as running `setupEventRecorder` followed by `startRecordingEvents` or
                            bool& playingEvents,                // `startPlayingEvents` (depending on which is passed in as `true`). This shortens code for tests.
                            const std::string& recordEventsFile,
                            const bool& overWriteRecordingFile,
                            const std::string& playEventsFile,
                            std::string&  errorMessage);
    void startRecordingEvents();                                // Start recording SDL events
    void setPauseRecordingEvents(const bool pause);             // Pause (or un-pause) recording SDL events
    bool stopRecordingEvents(std::string * errorMsg = nullptr,  // End recording SDL events. If error == true, then add ERROR to auto-logged files
                             const bool& error = false);
    bool recordingEvents();                                     // Returns true if SRE is recording SDL events
    void startPlayingEvents();                                  // Start playing recorded SDL events
    void setPausePlayingEvents(const bool pause);               // Pause (or un-pause) playback of recorded SDL events
    bool playingEvents();                                       // Returns true if SRE is playing back recorded SDL events
    bool playingEventsAborted();                                // Returns true if SDL event playback was aborted by the user
    void captureFrameToFile(RenderPass * renderPass,            // Capture image of frame generated by renderPass and write to path. If a multipass framebuffer is
                            std::filesystem::path path,         // attached to RenderPass, then set captureFromScreen = true. Finish RenderPass before calling.
                            bool captureFromScreen= false);
    void captureFrame(RenderPass * renderPass,                  // Capture image of frame generated by renderPass and store in memory. If a multipass framebuffer is
                      bool captureFromScreen= false);           // attached to RenderPass, then set captureFromScreen = true. Finish RenderPass before calling.
    int numCapturedImages();                                    // Returns the number of captured images saved so far
    void writeCapturedImages(std::string fileName);             // Write all the images stored in memory to files
    void writeImage(std::vector<glm::u8vec4> image,             // Write a single image to the specified path
                    glm::ivec2 imageDimensions,
                    std::filesystem::path path);
    void addCommentToEventsFile(std::string_view comment);      // Add a comment to the event file (only possible while recording)

    // Mouse and keyboard interface
    bool isKeyPressed(SDL_Keycode keyCode);                     // Return true if a specific key is pressed 
    bool isAnyKeyPressed();                                     // Return true if any key is pressed
    void setContextMenuActive(const bool& active);              // Tell SRE a context menu is active (needed to avoid false positives for userClickedOutsideModalTwice)

private:
    void frame(float deltaTimeSec);
    Renderer* r;
    SDLRenderer(const SDLRenderer&) = delete;
    bool m_turnedNavKeyboardOff = false;
    static bool UsingOpenGL_EGL();

    std::unique_ptr<VR> vr;
    std::string windowTitle;
    const float timePerFrame = 1.0f/60.0f;
    const float m_maxDeltaResponsiveTime = timePerFrame * 5.0f;
    Clock::time_point m_lastResponsiveTick;
    bool m_clickedOutsideModal = false;
    bool contextMenuActive = false;

    // Event loop control and execution
    bool running = false;
    bool runningEventSubLoop = false;
    void executeEventLoop(bool& runEventLoop); // Implementation of event loop
    void processEvents(std::vector<SDL_Event> events);
    void transformEventCoordinatesFromPointsToPixels(SDL_Event& e);
    void registerEvent(SDL_Event e);
    bool isHotKeyCombo(SDL_Event e);
    bool isHotKeyComboPanning();
    std::string GetKeyNameIfSpecial(SDL_Keycode key);

    // Window properties
    glm::ivec2 windowPosition = {SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED};
    int windowWidth = 800;
    int windowHeight = 600;
    bool minimized = false;
    SDL_Window *window = nullptr;

    float deltaTimeEvent;
    float deltaTimeUpdate;
    float deltaTimeRender;

    friend class SDLRendererInternal;
    friend class Inspector;

    // Minimal rendering option
    int frameNumber = 0;
    int lastEventFrameNumber = -99; // Invalid value to start
    bool appUpdated = false;
    bool minimalRendering = false;
    int minimumFramesNeededForImGuiDraw();

    // Handle mouse cursor changes
    bool imGuiHasCursor{};
    SDL_Cursor* cursor;
    SDL_Cursor* lastCursor;
    SDL_Cursor* arrowCursor;
    SDL_Cursor* waitCursor;
    SDL_Cursor* resizeAllCursor;
    void initMouseCursors();
    void setMouseCursorForImGui();

    // Recording and playing of frames and events
    bool isWindowHidden = false;
    void recordFrame();
    void recordEvent(const SDL_Event& e);
    SDL_Event getNextRecordedEvent(bool& endOfFile);
    bool readRecordedEvents(const std::string& fileName, std::string& errorMessage);
    std::stringstream GetSettingsAndAdvanceEventsStreamIfAble(
                                                   const std::string& currentLine,
                                                   std::ifstream* eventsFilePtr);
    std::vector<SDL_Event> getRecordedEventsForNextFrame();
    bool pushNextRecordedEventToSDL(bool endOfFile);
    int nextRecordedFramePeek();
    std::string m_jsonSettings = "";
    bool m_autoRecordEvents = false;
    bool m_recordingEventsRequested = false;
    bool m_recordingEvents = false;
    bool m_playingBackEvents = false;
    bool m_playingBackEventsAborted = false;
    std::string m_recordingFileName;
    std::stringstream m_eventsFileHeaderStream;
    std::stringstream m_recordingStream;
    std::stringstream m_playbackStream;
    size_t m_imGuiIniFileSize = 0;
    const char * m_imGuiIniFileCharPtr = nullptr;
    int m_playbackFrame = -99; // Invalid value to start
    bool m_pausePlaybackOfEvents = false;
    bool m_pauseRecordingOfEvents = false;
    bool m_writingImages = false;
    std::vector<std::vector<glm::u8vec4>> m_image;
    std::vector<glm::ivec2> m_imageDimensions;
    bool m_mouseDown = false;
    std::vector<SDL_Keycode> keyPressed;
    void addKeyPressed(SDL_Keycode keyCode);
    void removeKeyPressed(SDL_Keycode keyCode);
    bool m_loggedUserMousePosInPlayback;
    int m_numTimesMaxMouseMotionExeededForPlayback;
    int m_lastFrameMouseMotionExceededForPlayback;
    void ManageMouseMotionLoggingForPlayback();
    void ResetMouseMotionLoggingForPlayback();
    glm::ivec2 m_userMousePosInPlayback;
};

}
