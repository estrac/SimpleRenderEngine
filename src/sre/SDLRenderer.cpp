/*
 *  SimpleRenderEngine (https://github.com/mortennobel/SimpleRenderEngine)
 *
 *  Created by Morten Nobel-Jørgensen ( http://www.nobel-joergensen.com/ )
 *  License: MIT
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <flags/flags.h> // Utility to read command line options
#include <imgui_impl_sdl2.h>
#include <sre/Log.hpp>
#include <sre/VR.hpp>
#include <imgui.h>
#include <picojson.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <sre/SDLRenderer.hpp>
#define SDL_MAIN_HANDLED


#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif
#include <sre/impl/GL.hpp>

#ifdef SRE_DEBUG_CONTEXT
void GLAPIENTRY openglCallbackFunction(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam) {
    using namespace std;
    const char* typeStr;
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        typeStr = "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeStr = "DEPRECATED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeStr = "UNDEFINED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeStr = "PORTABILITY";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeStr = "PERFORMANCE";
        break;
    case GL_DEBUG_TYPE_OTHER:
    default:
        typeStr = "OTHER";
        break;
        }
    const char* severityStr;
    switch (severity) {
    case GL_DEBUG_SEVERITY_LOW:
        severityStr = "LOW";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        severityStr = "MEDIUM";
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        severityStr = "HIGH";
        break;
    default:
        severityStr = "Unknown";
        break;
    }
    LOG_ERROR("---------------------opengl-callback-start------------\n"
              "message: %s\n"
              "type: %s\n"
              "id: %i\n"
              "severity: %s\n"
              "---------------------opengl-callback-end--------------"
              ,message,typeStr, id ,severityStr
    );

        }
#endif

namespace sre{

    SDLRenderer* SDLRenderer::instance = nullptr;

    struct SDLRendererInternal{
        static void update(float f){
            SDLRenderer::instance->frame(f);
        }
    };

    void update(){
        using FpSeconds = std::chrono::duration<float, std::chrono::seconds::period>;
        static auto lastTick = Clock::now();
        auto tick = Clock::now();
        float deltaTime = std::chrono::duration_cast<FpSeconds>(tick - lastTick).count();
        lastTick = tick;
        SDLRendererInternal::update(deltaTime);
    }

    SDLRenderer::SDLRenderer()
    :frameUpdate ([](float){}),
     frameRender ([](){}),
     windowMaximized ([](){}),
     windowRestored ([](){}),
     windowSizeChanged ([](){}),
     stopProgram ([this](){stopEventLoop();}),
     keyEvent ([](SDL_Event&){}),
     mouseEvent ([](SDL_Event&){}),
     controllerEvent ([](SDL_Event&){}),
     joystickEvent ([](SDL_Event&){}),
     touchEvent ([](SDL_Event&){}),
     otherEvent([](SDL_Event&){}),
     windowTitle( std::string("SimpleRenderEngine ")+std::to_string(Renderer::sre_version_major)+"."+std::to_string(Renderer::sre_version_minor )+"."+std::to_string(Renderer::sre_version_point))
    {

        instance = this;

    }

    SDLRenderer::~SDLRenderer() {
        delete r;
        r = nullptr;

        instance = nullptr;

        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void SDLRenderer::frame(float deltaTimeSec){
        using MilliSeconds = std::chrono::duration<float, std::chrono::milliseconds::period>;
        auto lastTick = Clock::now();

        getAndProcessEvents();

        if (minimized) return;

        // Determine whether to render frame for "minimalRendering" option
        bool shouldRenderFrame = true;
        if (minimalRendering) {
            if (appUpdated || isAnyKeyPressed() || m_mouseDown) {
                if (m_recordingEvents && !m_pauseRecordingOfEvents
                                         && lastEventFrameNumber != frameNumber) {

                    // Record frame for app update or if any key is pressed
                    // (unless event is already recorded)
                    recordFrame();
                }
                lastEventFrameNumber = frameNumber;
                if (appUpdated) {
                    appUpdated = false;
                }
            }
            if (frameNumber > lastEventFrameNumber
                                            + minimumFramesNeededForImGuiDraw()) {
                shouldRenderFrame = false;
            }
        }
        // Update and draw frame, measure times, and swap window
        {   // Measure time for event processing
            auto tick = Clock::now();
            deltaTimeEvent = std::chrono::duration_cast<MilliSeconds>(tick - lastTick).count();
            lastTick = tick;
        }
        if (shouldRenderFrame) {
            frameUpdate(deltaTimeSec);
            {   // Measure time for updating the frame
                auto tick = Clock::now();
                deltaTimeUpdate = std::chrono::duration_cast<MilliSeconds>(tick - lastTick).count();
                lastTick = tick;
            }
            frameRender();
            {   // Measure time for rendering the frame
                auto tick = Clock::now();
                deltaTimeRender = std::chrono::duration_cast<MilliSeconds>(tick - lastTick).count();
                lastTick = tick;
            }
            if (m_recordingEvents && !m_pauseRecordingOfEvents
                                     && frameNumber > lastEventFrameNumber
                                     && frameNumber <= lastEventFrameNumber
                                        + minimumFramesNeededForImGuiDraw()) {
                recordFrame();
            }
            r->swapWindow();
            frameNumber++;
        } else {
            deltaTimeUpdate = 0.0f;
            deltaTimeRender = 0.0f;
        }
    }

    int SDLRenderer::minimumFramesNeededForImGuiDraw() {
        // Draw at least two frames after each event: one to allow ImGui to handle
        // the event and one to process actions triggered by ImGui (e.g. draw #1
        // draws pressed down OK button, draw #2 hides the window and executes
        // actions resulting from OK).
        // When a PopupModal is activated, ImGui uses 10 frames to "fade" in a
        // transparent gray layer behind the dialog to put focus on it.
        if (ImGui::IsAnyPopupModalActive()) return 10;
        else return 6; // ImGui may need up to 9 frames to resize windows
    }

    void SDLRenderer::getAndProcessEvents() {
        SDL_Event event;
        std::vector<SDL_Event> events;
        if (!m_playingBackEvents) {
            // Normal code execution path
            while(SDL_PollEvent(&event) != 0) {
                events.push_back(event);
            }
        } else if (!m_pausePlaybackOfEvents) {
            // Code execution path while playing events
            while(SDL_PollEvent(&event) != 0) {
                // Check if user wants to take back control of the mouse
                if (event.type == SDL_MOUSEMOTION) {
                    // Note: deliberately don't scale coords to pixels
                    if (m_loggedUserMousePosInPlayback) {
                        float dx = event.motion.x - m_userMousePosInPlayback.x;
                        float dy = event.motion.y - m_userMousePosInPlayback.y;
                        float maxMouseMotion = std::max(abs(dx), abs(dy));
                        if (maxMouseMotion > 10.0f) {
                            m_numTimesMaxMouseMotionExeededForPlayback++;
                            m_lastFrameMouseMotionExceededForPlayback = frameNumber;
                        }
                    }
                    m_userMousePosInPlayback = {event.motion.x, event.motion.y};
                    m_loggedUserMousePosInPlayback = true;
                }
                if (!m_playingBackEventsAborted
                           && m_numTimesMaxMouseMotionExeededForPlayback > 2) {
                    // User moved mouse aggressively 3 frames -- abort playback
                    // (3 frames to avoid spurious mouse events on some OS's)
                    m_playingBackEventsAborted = true;
                    bool endOfFile = false;
                    while ((isAnyKeyPressed() || m_mouseDown) && !endOfFile) {
                        // Ensure that no keys are left in a "pressed" state
                        // and that mouse is not left in a "down" state
                        event = getNextRecordedEvent(endOfFile);
                        // Since user wants to take back control, only process the
                        // needed key and mouse events
                        auto key = event.key.keysym.sym;
                        if((event.type == SDL_KEYUP && isKeyPressed(key))
                            || (event.type == SDL_MOUSEBUTTONUP && m_mouseDown)) {
                            processEvents({event});
                        }
                    }
                    // Erase the playback stream
                    std::stringstream().swap(m_playbackStream);
                }
            }
            events = getRecordedEventsForNextFrame();
            ManageMouseMotionLoggingForPlayback();
        }

        processEvents(events);

        if (m_playingBackEvents) {
            while( SDL_PollEvent(&event) != 0 ) {
                // Clear event queue from playback events so that new events
                // are recognized
            }
        }
    }

    // Ensure that no keys are left in a "pressed" state and that mouse is not
    // left in a "down" state
    bool SDLRenderer::processKeyPressedAndMouseDownEvents(
                                                     std::string * errorMessage) {
        int counter = 0;
        while ((isAnyKeyPressed() || m_mouseDown)) {
            SDL_Event event;
            std::vector<SDL_Event> events;
            if (!m_playingBackEvents) {
                // Normal code execution path
                while(SDL_PollEvent(&event) != 0) {
                    events.push_back(event);
                }
            } else if (!m_pausePlaybackOfEvents) {
                // Code execution path while playing events
                events = getRecordedEventsForNextFrame();
            }
            frameNumber++;
    
            processEvents(events);
    
            counter++;
            if (counter > 30) { // Equivalent to 3 second wait
                if (errorMessage != nullptr) {
                    std::ostringstream info;
                    info << "Events are still in 'pressed' or 'down' state. "
                         << "This can cause severe issues in ImGui. Events are:";
                    for (auto key : keyPressed) {
                        info << std::endl << "    keyCode = " << key;
                    }
                    if (m_mouseDown) info << std::endl << "    mouse is down";
                    *errorMessage = info.str();
                }
                return false;
            }
            if (!m_playingBackEvents && (isAnyKeyPressed() || m_mouseDown)) {
                SDL_Delay(100); // Delay 100 milliseconds (1/10 of a second)
            }
        }
        return true;
    }

    void SDLRenderer::processEvents(std::vector<SDL_Event> events) {
        SDL_Event e;
        for (int  i = 0; i < events.size(); i++) {
            e = events[i];
            lastEventFrameNumber = frameNumber;
            if (m_recordingEvents && !m_pauseRecordingOfEvents ) {
                recordEvent(e);
                if (events.size() > 1) {
                    // Don't allow multiple events per frame in recorded file.
                    // This is slightly slower, but much more predictable.
                    lastEventFrameNumber = ++frameNumber;
                }
            }
            registerEvent(e);
            ImGuiIO& io = ImGui::GetIO();
            if (isHotKeyComboPanning()) {
                // Turn ImGui keyboard navigation off if panning with hotkeys (because this conflicts with navigation)
                if (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) {
                    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
                    m_turnedNavKeyboardOff = true;
                }
            } else if (m_turnedNavKeyboardOff) {
                // Turn ImGui keyboard navigation back on if it was turned off
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                m_turnedNavKeyboardOff = false;
            }
            transformEventCoordinatesFromPointsToPixels(e);
            ImGui_ImplSDL2_ProcessEvent(&e);
            switch (e.type) {
                case SDL_QUIT:
                    {
                        // Execute user callback
                        stopProgram();
                        break;
                    }
                case SDL_WINDOWEVENT:
                    {
                        /* TODO: (#19) The SIZE_CHANGED and RESIZED events are not called while the mouse is still being held down,
                         * even though the window size has already changed, leaving un-drawn parts of the window. If the output from
                         * the `getWindowSizeInPixels()` function is printed every frame, then it stops when resizing or moving.
                         * Check if this is still the case after transitioning to SDL3. If so, investigate to see if there is a flag
                         * that can be passed to SDL or if something is set up wrong. If not, put in an issue with SDL3.
                        if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                            std::cout << "=== Window size changed to " << e.window.data1 << ", " << e.window.data2
                                      << ", Pixels = " << getWindowSizeInPixels().x << ", "  << getWindowSizeInPixels().y << std::endl;
                        }
                        if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                            std::cout << "=== Window resized to " << e.window.data1 << ", " << e.window.data2
                                      << ", Pixels = " << getWindowSizeInPixels().x << ", "  << getWindowSizeInPixels().y << std::endl;
                        }*/
                        if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                            if (m_playingBackEvents) {
                                setWindowSize({e.window.data1, e.window.data2});
                                ResetMouseMotionLoggingForPlayback();
                            }
                            windowSizeChanged();
                        }
                        if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                            if (m_playingBackEvents) {
                                SDL_MinimizeWindow(window);
                                ResetMouseMotionLoggingForPlayback();
                            }
                            minimized = true;				    
                        }
                        if (e.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
                            if (m_playingBackEvents) {
                                // TODO: (#19) SDL_MaximizeWindow(window)
                                // should be used here but does not work with
                                // SDL2. SDL3 provides SDL_SyncWindow() to apply
                                // the new state (see SDL3 docs on MaximizeWindow).
                                // Try using SDL_MaximizeWindow for #19.
                                SDL_MaximizeWindow(window);
                                // The following is needed if maximizing on a
                                // larger screen then used for recording
                                // Unfortunately, it looks like this needs to be
                                // called after the next frame.
                                setWindowSize({e.window.data1, e.window.data2});
                                // If have issues, use SDL_GetWindowPosition to
                                // get the position and write out in comment for
                                // maximize event, then use setWindowPosition to
                                // move it
                                // setWindowPosition({0, 0}); // Needed for MSWindows
                                ResetMouseMotionLoggingForPlayback();
                            }
                            windowMaximized();
                        }
                        if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
                            if (m_playingBackEvents) {
                                // TODO: (#19) SDL_RestoreWindow(window)
                                // should be used here but does not work with
                                // SDL2. SDL3 provides SDL_SyncWindow() to apply
                                // the new state (see SDL3 docs on RestoreWindow).
                                // Try using SDL_RestoreWindow for #19.
                                SDL_RestoreWindow(window);
                                // The following is needed if maximizing on a
                                // larger screen then used for recording
                                // Unfortunately, it looks like this needs to be
                                // called after the next frame.
                                setWindowSize({e.window.data1, e.window.data2});
                                ResetMouseMotionLoggingForPlayback();
                            }
                            if (minimized) minimized = false;
                            windowRestored();
                        }
                    }
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    {
                        if (!io.WantCaptureKeyboard || isHotKeyCombo(e)) {
                            // Pass event to user callback
                            keyEvent(e);
                        }
                        break;
                    }
                case SDL_MOUSEMOTION:
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEWHEEL:
                    {
                        if (m_playingBackEvents && !isWindowHidden) {
                            // Warp the mouse (required for playback to be successful when window is not hidden)
                            int mouse_x, mouse_y;
                            if (e.type == SDL_MOUSEMOTION) {
                                mouse_x = e.motion.x;
                                mouse_y = e.motion.y;
                            } else if (e.type == SDL_MOUSEBUTTONDOWN
                                               || e.type == SDL_MOUSEBUTTONUP) {
                                mouse_x = e.button.x;
                                mouse_y = e.button.y;
                            } else { // (e.type == SDL_MOUSEWHEEL) {
                                mouse_x = e.wheel.x;
                                mouse_y = e.wheel.y;
                            }
                            // Mouse warping causes playback to abort on Windows
                            // and mouse is not in right location on MacOS Apple
                            // (MacOS may be fixed when display scaling works).
                            // TODO: make OpenGL utility to draw mouse location
                            //SDL_WarpMouseInWindow(window, mouse_x, mouse_y);
                        }
                        if (!io.WantCaptureMouse) {
                            // Pass event to user callback
                            mouseEvent(e);
                        }
                        break;
                    }
                case SDL_CONTROLLERAXISMOTION:
                case SDL_CONTROLLERBUTTONDOWN:
                case SDL_CONTROLLERBUTTONUP:
                case SDL_CONTROLLERDEVICEADDED:
                case SDL_CONTROLLERDEVICEREMOVED:
                case SDL_CONTROLLERDEVICEREMAPPED:
                    {
                        // Pass event to user callback
                        controllerEvent(e);
                        break;
                    }
                case SDL_JOYAXISMOTION:
                case SDL_JOYBALLMOTION:
                case SDL_JOYHATMOTION:
                case SDL_JOYBUTTONDOWN:
                case SDL_JOYBUTTONUP:
                case SDL_JOYDEVICEADDED:
                case SDL_JOYDEVICEREMOVED:
                    {
                        // Pass event to user callback
                        joystickEvent(e);
                        break;
                    }
                case SDL_FINGERDOWN:
                case SDL_FINGERUP:
                case SDL_FINGERMOTION:
                    {
                        // Pass event to user callback
                        touchEvent(e);
                        break;
                    }
                default:
                    {
                        // Pass event to user callback
                        otherEvent(e);
                        break;
                    }
            }
        }
    }

    void SDLRenderer::transformEventCoordinatesFromPointsToPixels(SDL_Event& e) {
        // TODO: (#19) If SDL3 honors choice of scaled vs. pixels for mouse
        //             coordinates, then this function will not be necessary.
        switch (e.type) {
            case SDL_MOUSEMOTION:
            {
                float displayScale = getDisplayScale();
                e.motion.x *= displayScale;
                e.motion.y *= displayScale;
                e.motion.xrel *= displayScale;
                e.motion.yrel *= displayScale;
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            {
                float displayScale = getDisplayScale();
                e.button.x *= displayScale;
                e.button.y *= displayScale;
                break;
            }
            case SDL_MOUSEWHEEL:
            {
                float displayScale = getDisplayScale();
                e.wheel.x *= displayScale;
                e.wheel.y *= displayScale;
                break;
            }
            case SDL_CONTROLLERAXISMOTION:
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED:
            case SDL_CONTROLLERDEVICEREMAPPED:
                LOG_ERROR("Controller coordinates not scaled to content scale as requested.");
                break;
            case SDL_JOYAXISMOTION:
            case SDL_JOYBALLMOTION:
            case SDL_JOYHATMOTION:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                LOG_ERROR("Joystick coordinates not scaled to content scale as requested.");
                break;
            case SDL_FINGERDOWN:
            case SDL_FINGERUP:
            case SDL_FINGERMOTION:
            {
                float displayScale = getDisplayScale();
                e.tfinger.x *= displayScale;
                e.tfinger.y *= displayScale;
                e.tfinger.dx *= displayScale;
                e.tfinger.dy *= displayScale;
                break;
            }
            default:
            {
                // Do nothing
                break;
            }
        }
    }

    void SDLRenderer::registerEvent(SDL_Event e) {
        // Register pressed keys
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            auto keyState = e.key.state;
            auto key = e.key.keysym.sym;
            if (keyState == SDL_PRESSED) {
                addKeyPressed(key);
            } else {
                removeKeyPressed(key);
            }
        }
        // Register pressed mouse button
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            m_mouseDown = true;
        } else if (e.type == SDL_MOUSEBUTTONUP) {
            m_mouseDown = false;
        }
    }

    bool SDLRenderer::isHotKeyCombo(SDL_Event e) {

        auto keyState = e.key.state;
        auto key = e.key.keysym.sym;

        bool keyUpOrDown = e.type == SDL_KEYDOWN || e.type == SDL_KEYUP;
        if (keyUpOrDown && (key == SDLK_F1 || key == SDLK_F2 || key == SDLK_F3 || key == SDLK_F4  || key == SDLK_F5  || key == SDLK_F6
                         || key == SDLK_F7 || key == SDLK_F8 || key == SDLK_F9 || key == SDLK_F10 || key == SDLK_F11 || key == SDLK_F12
                         || key == SDLK_ESCAPE)) {
            return true;
        }

        bool shiftDown = isKeyPressed(SDLK_LSHIFT) || isKeyPressed(SDLK_RSHIFT);
        // Hotkey combination used for exiting the program
        if (shiftDown && keyUpOrDown && key == SDLK_BACKSPACE) return true;

        bool ctrlDown = isKeyPressed(SDLK_LCTRL) || isKeyPressed(SDLK_RCTRL);

        // Hotkey combinations used for zooming 
        if (ctrlDown && (isKeyPressed(SDLK_MINUS) || isKeyPressed(SDLK_EQUALS))) return true;

        if (isHotKeyComboPanning()) return true;

        return false;
    }

    bool SDLRenderer::isHotKeyComboPanning() {
        bool ctrlDown = isKeyPressed(SDLK_LCTRL) || isKeyPressed(SDLK_RCTRL);
        // Hotkey combinations used for panning
        if (ctrlDown && (isKeyPressed(SDLK_UP) || isKeyPressed(SDLK_DOWN) || isKeyPressed(SDLK_LEFT) || isKeyPressed(SDLK_RIGHT))) {
            return true;
        }
        return false;
    }

    std::string SDLRenderer::GetKeyNameIfSpecial(SDL_Keycode key) {
    // Return a label for special (non-alphanumeric) keys, else return ""
        std::string name;
        switch (key) {
            case SDLK_F1:
                name = " (F1)";
                break;
            case SDLK_F2:
                name = " (F2)";
                break;
            case SDLK_F3:
                name = " (F3)";
                break;
            case SDLK_F4:
                name = " (F4)";
                break;
            case SDLK_F5:
                name = " (F5)";
                break;
            case SDLK_F6:
                name = " (F6)";
                break;
            case SDLK_F7:
                name = " (F7)";
                break;
            case SDLK_F8:
                name = " (F8)";
                break;
            case SDLK_F9:
                name = " (F9)";
                break;
            case SDLK_F10:
                name = " (F10)";
                break;
            case SDLK_F11:
                name = " (F11)";
                break;
            case SDLK_F12:
                name = " (F12)";
                break;
            case SDLK_RETURN:
                name = " (ENTER)";
                break;
            case SDLK_TAB:
                name = " (TAB)";
                break;
            case SDLK_ESCAPE:
                name = " (ESCAPE)";
                break;
            case SDLK_LCTRL:
                name = " (LCTRL)";
                break;
            case SDLK_RCTRL:
                name = " (RCTRL)";
                break;
            case SDLK_LSHIFT:
                name = " (LSHIFT)";
                break;
            case SDLK_RSHIFT:
                name = " (RSHIFT)";
                break;
            case SDLK_BACKSPACE:
                name = " (BACKSPACE)";
                break;
            case SDLK_LEFT:
                name = " (LEFT)";
                break;
            case SDLK_RIGHT:
                name = " (RIGHT)";
                break;
            case SDLK_UP:
                name = " (UP)";
                break;
            case SDLK_DOWN:
                name = " (DOWN)";
                break;
            default:
                name = "";
        }
        return name;
    }

    void SDLRenderer::startEventLoop() {
        if (!window){
            LOG_INFO("SDLRenderer::init() not called");
        }
        running = true;
        executeEventLoop(running);
    }

    void SDLRenderer::stopEventLoop() {
        running = false;
        runningEventSubLoop = false;
        if (m_recordingEvents) {
            std::string errorMessage;
            if (!stopRecordingEvents(&errorMessage)) {
                LOG_ERROR(errorMessage.c_str());
            }
        }
    }

    void SDLRenderer::startEventSubLoop() {
        if (!running) return; // Only allow start as a sub-loop
        if (runningEventSubLoop)
            LOG_INFO("Multiple simultaneous render sub-loops attempted");
        else
        {
            runningEventSubLoop = true;
            executeEventLoop(runningEventSubLoop);
        }
    }

    void SDLRenderer::stopEventSubLoop() {
        runningEventSubLoop = false;
    }

    void SDLRenderer::executeEventLoop(bool& runEventLoop) {
#ifdef EMSCRIPTEN
        emscripten_set_main_loop(update, 0, 1);
#else
        using FpSeconds = std::chrono::duration<float, std::chrono::seconds::period>;
        auto lastTick = Clock::now();
        float deltaTime = 0;

        while (runEventLoop){
            frame(deltaTime);

            auto tick = Clock::now();
            deltaTime = std::chrono::duration_cast<FpSeconds>(tick - lastTick).count();

            while (deltaTime < timePerFrame){
                Uint32 delayMs;
                float delayS = timePerFrame - deltaTime;
                if (!minimalRendering)
                {   // Match frame rate exactly by underestimating delay
                    // The while loop will fill the < 1 Millisecond gap
                    delayMs = static_cast<Uint32>(delayS * 1000.0f);
                }
                else
                {   // For minimal CPU use, overestimate delay by <= 1 Ms
                    delayMs = static_cast<Uint32>(delayS * 1000.0f + 1);
                }
                SDL_Delay(delayMs);
                tick = Clock::now();
                deltaTime = std::chrono::duration_cast<FpSeconds>(tick - lastTick).count();
            }
            lastTick = tick;
        }
#endif
    }

    void SDLRenderer::startTimerForKeepAppResponsive() {
        m_lastResponsiveTick = Clock::now();
    }

    void SDLRenderer::keepAppResponsive() {
        using FpSeconds = std::chrono::duration<float, std::chrono::seconds::period>;

        auto tick = Clock::now();
        auto deltaResponsiveTime = std::chrono::duration_cast<FpSeconds>(tick - m_lastResponsiveTick).count();

        if (deltaResponsiveTime > m_maxDeltaResponsiveTime) {
            // Minimal rendering option will prevent drawing when no keyboard or
            // mouse events have been detected, so set appUpdated to true
            SetAppUpdated(true);
            frame(deltaResponsiveTime);
            m_lastResponsiveTick = tick;
        }
    }

    void SDLRenderer::drawFrame() {
        if (minimized) return;
        frameUpdate(0);
        frameRender();
        frameNumber++;
        r->swapWindow();
    }

    int SDLRenderer::getFrameNumber() {
        return frameNumber;
    }

    void SDLRenderer::startEventLoop(std::shared_ptr<VR> vr) {
        if (!window){
            LOG_INFO("SDLRenderer::init() not called");
        }

        running = true;

        typedef std::chrono::high_resolution_clock Clock;
        using FpSeconds = std::chrono::duration<float, std::chrono::seconds::period>;
        auto lastTick = Clock::now();
        float deltaTime = 0;

        while (running){
            vr->render();
            frame(deltaTime);

            auto tick = Clock::now();
            deltaTime = std::chrono::duration_cast<FpSeconds>(tick - lastTick).count();
            lastTick = tick;
        }
    }

    void SDLRenderer::setWindowPosition(glm::ivec2 position) {
        windowPosition = position;
        if (window!= nullptr){
            SDL_SetWindowPosition(window, windowPosition.x, windowPosition.y);
        }
    }

    void SDLRenderer::setWindowSize(glm::ivec2 size) {
        int width = size.x;
        int height = size.y;
        windowWidth = width;
        windowHeight = height;
        if (window!= nullptr){
            SDL_SetWindowSize(window, width, height);
        }
    }

    glm::ivec2 SDLRenderer::getWindowSize() {
        return r->getWindowSize();
    }

    glm::ivec2 SDLRenderer::getWindowSizeInPixels() {
        return r->getWindowSizeInPixels();
    }

    glm::ivec2 SDLRenderer::getDrawableSize() {
        return r->getDrawableSize();
    }

    float SDLRenderer::getDisplayScale() {
        return (float)r->getWindowSizeInPixels().y / (float)r->getWindowSize().y;
    }

    void SDLRenderer::setWindowTitle(std::string title) {
        windowTitle = title;
        if (window != nullptr) {
            SDL_SetWindowTitle(window, title.c_str());
        }
    }

    SDL_Window *SDLRenderer::getSDLWindow() {
        return window;
    }

    void SDLRenderer::setFullscreen(bool enabled) {
#ifndef EMSCRIPTEN
        if (isFullscreen() != enabled){
            Uint32 flags = (SDL_GetWindowFlags(window) ^ SDL_WINDOW_FULLSCREEN_DESKTOP);
            if (SDL_SetWindowFullscreen(window, flags) < 0) // NOTE: this takes FLAGS as the second param, NOT true/false!
            {
                std::cerr << "Toggling fullscreen mode failed: " << SDL_GetError() << std::endl;
                return;
            }
        }
#endif
    }

    bool SDLRenderer::isFullscreen() {
        return ((SDL_GetWindowFlags(window)&(SDL_WINDOW_FULLSCREEN|SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0);
    }

    void SDLRenderer::setMouseCursorVisible(bool enabled) {
        SDL_ShowCursor(enabled?SDL_ENABLE:SDL_DISABLE);
    }

    bool SDLRenderer::isMouseCursorVisible() {
        return SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE;
    }

    bool SDLRenderer::setMouseCursorLocked(bool enabled) {
        if (enabled){
            setMouseCursorVisible(false);
        }
        return SDL_SetRelativeMouseMode(enabled?SDL_TRUE:SDL_FALSE) == 0;
    }

    bool SDLRenderer::isMouseCursorLocked() {
        return SDL_GetRelativeMouseMode() == SDL_TRUE;
    }

    SDLRenderer::InitBuilder SDLRenderer::init() {
        return SDLRenderer::InitBuilder(this);
    }

    glm::vec3 SDLRenderer::getLastFrameStats() {
        return {
                deltaTimeEvent,deltaTimeUpdate,deltaTimeRender
        };
    }

    void SDLRenderer::SetArrowCursor() {
        SDL_FreeCursor(cursor);
        cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        cursorType = Cursor::Arrow;
        SDL_SetCursor(cursor);
    }

    void SDLRenderer::Begin(Cursor cursorStart) {
        if (cursor != NULL) {
            if (cursorType != Cursor::Arrow)
                LOG_ERROR("Last mouse cursor not freed in SDLRenderer::Begin");
            SDL_FreeCursor(cursor);
        }
        switch (cursorStart) {
            case Cursor::Arrow:
                cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
                break;
            case Cursor::Wait:
                cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
                break;
            case Cursor::Hand:
                cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
                break;
            case Cursor::SizeAll:
                cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
                break;
            default:
                LOG_ERROR("Invalid mouse cursor passed to SDLRenderer::Begin");
                return;
        }
        cursorType = cursorStart;
        SDL_SetCursor(cursor);
    }

    void SDLRenderer::End(Cursor cursorEnd) {
        if (cursorEnd != cursorType && cursorType != Cursor::Arrow)
            LOG_ERROR("Ending cursor not same as starting cursor in SDLRenderer");
        SetArrowCursor();
    }

    void SDLRenderer::SetMinimalRendering(bool minimalRendering) {
        this->minimalRendering = minimalRendering;
    }

    void SDLRenderer::SetAppUpdated(bool appUpdated) {
        this->appUpdated = appUpdated;
    }

    bool SDLRenderer::setupEventRecorder(bool& recordingEvents,
                                         bool& playingEvents,
                                         const std::string& recordEventsFileName,
                                         const bool& overWriteRecordingFile,
                                         const std::string& playEventsFileName,
                                         std::string& errorMessage) {
        bool recordEventsToLog = false;
        if (!recordingEvents && m_autoRecordEvents) recordEventsToLog = true;
        if (recordingEvents || recordEventsToLog) {
            if (m_recordingEvents) {
                errorMessage = "Attempted to record events while already recording";
                recordingEvents = false;
                return false;
            }
            if (recordEventsToLog) {
                m_recordingFileName = Log::GetEventsArchivePath().string();
            } else {
                m_recordingFileName = recordEventsFileName;
            }
            if (std::filesystem::exists(m_recordingFileName)) {
                if (!overWriteRecordingFile) {
                    std::stringstream errorStream;
                    errorStream << "Specified recording file '"
                        << m_recordingFileName << "' exists. Please move or"
                        << " delete the file." << std::endl;
                    errorMessage = errorStream.str();
                    recordingEvents = false;
                    return false;
                } else { // overWriteRecordingFile
                    if (!std::filesystem::remove(m_recordingFileName)) {
                        std::stringstream errorStream;
                        errorStream << "Specified recording file '"
                            << m_recordingFileName
                            << "' could not be removed. Please move, delete, or"
                            << " change permissions of the file." << std::endl;
                        errorMessage = errorStream.str();
                        recordingEvents = false;
                        return false;
                    }
                }
            }
            // Test whether file can be opened for writing
            std::ofstream outFile(m_recordingFileName, std::ios::out);
            if(!outFile) {
                std::stringstream errorStream;
                errorStream << "Specified recording file '" << m_recordingFileName
                    << "' could not be opened for writing." << std::endl;
                errorMessage = errorStream.str();
                recordingEvents = false;
                return false;
            } else {
                outFile.close();
            }
            std::stringstream().swap(m_recordingStream);
            m_recordingEventsRequested = true; // Set true if requested or logging
        }
        if (playingEvents) {
            if (m_playingBackEvents) {
                errorMessage = "Attempted to play events while already playing";
                playingEvents = false;
                return false;
            }
            if (!readRecordedEvents(playEventsFileName, errorMessage)) {
                playingEvents = false;
                return false;
            }
        }
        return true;
    }

    bool SDLRenderer::setupAndStartEventRecorder(bool& recordingEvents,
                                         bool& playingEvents,
                                         const std::string& recordEventsFile,
                                         const bool& overWriteRecordingFile,
                                         const std::string& playEventsFile,
                                         std::string& errorMessage) {
        if (setupEventRecorder(recordingEvents, playingEvents, recordEventsFile,
                               overWriteRecordingFile, playEventsFile,
                               errorMessage)) {
            if (recordingEvents) {
                startRecordingEvents();
            }
            if (playingEvents) {
                startPlayingEvents();
            }
            return true;
        } else {
            return false;
        }
    }

    void SDLRenderer::startRecordingEvents() {
        if (m_recordingEvents) return;

        m_recordingEvents = true;
        // Load and save ini file for starting state (ImGui checks if already loaded)
        const char * iniFilename = ImGui::GetIO().IniFilename;
        if (iniFilename != nullptr) {
            ImGui::LoadIniSettingsFromDisk(iniFilename);
            m_imGuiIniFileCharPtr = ImGui::AllocateString(
                             ImGui::SaveIniSettingsToMemory(&m_imGuiIniFileSize));
        }
        if (m_playingBackEvents) frameNumber = -1; // Make numbering in output same
    }

    void SDLRenderer::recordFrame() {
        std::stringstream label;
        label << "#no event";
        m_recordingStream << frameNumber << " " << label.str() << std::endl;
    }

    // Record SDL events (mouse, keyboard, etc.) to "m_recordingStream" member
    // variable and write to file in ::stopRecordingEvents(). Can read later to
    // play back events, which enables testing scripts for ImGui interface
    void SDLRenderer::recordEvent(const SDL_Event& e) {
        // SDL Events  are defined in $SDL_HOME/include/SDL_events.h
        //   typedef union SDL_Event ...
        // SDL_Keysym is defined in $SDL_HOME/include/SDL_keyboard.h
        //   typedef struct SDL_Keysym ...
        // SDL_Scancode is defined in $SDL_HOME/include/SDL_scancode.h
        //   typedef enum { SDL_SCANCODE_UNKNOWN = 0, ... } SDL_Scancode;
        //   i.e. integer (same as enums)
        // SDL_Keycode is defined in $SDL_HOME/include/SDL_keycode.h
        //   typedef Sint32 SDL_Keycode [i.e. signed (standard) int (integer)]
        //   typedef enum {KMOD_NONE=0x0000, KMOD_LSHIF=0x0001, ...} SDL_Keymod
        //     [which is returned by SDL_GetModState()] - state of 'mod' keys
        // SDL types (like Uint8) are defined in $SDL_HOME/include/SDL_stdinc.h
        //   typedef uint8_t Uint8 // An unsigned 8-bit integer, type = 1 byte
        //   typedef uint16_t Uint16 // An unsigned 16-bit integer
        //   typedef int32_t Sint32; // A signed 32-bit integer
        // In C, uint8_t is defined in header stdint.h, guaranteed to be 8 bits
        //   typedef unsigned char uint8_t;
        //   typedef unsigned short uint16_t;
        //   typedef unsigned long uint32_t;
        //   typedef unsigned long long uint64_t;
        //   typedef int int32_t;
        // To properly print an unsigned char that is used to store bits (e.g.
        // that is not being used to store characters), you need to "promote"
        // the value using the bit operation "+" (needed for all 'Uint8', i.e.
        // unsigned, 8-bit integer) variables in the event structs). An
        // alternative would be to print in hexadecimal format (example below).
        // See discussion in StackOverflow:
        // [https://]
        // stackoverflow.com/questions/15585267/cout-not-printing-unsigned-char
        // This is the example for printing in hexadecimal format:
        // std::cout << std::showbase // show the 0x prefix
        //      << std::internal      // fill between the prefix and the number
        //      << std::setfill('0'); // fill with 0s
        // std::cout << std::hex << std::setw(4) << value

        // SDL provides the SDL_GetMouseState and SDL_GetModState functions that
        // a user can call at any time -- the information to support this call
        // during playback is stored right after the frame number for all events.
        // Note that sometimes the values returned by SDL_GetMouseState are
        // different than what is provided by the various mouse events. Storing
        // the mouse state separately allows playback to provide exactly what the
        // code experienced during recording of events.
        int x, y;
        m_recordingStream << frameNumber << " ";
        switch (e.type) {
            case SDL_QUIT:
                m_recordingStream
                    << e.quit.type << " "
                    << e.quit.timestamp << " "
                    << "#quit (end program)"
                    << std::endl;
                break;
            case SDL_WINDOWEVENT:
                m_recordingStream
                    << e.window.type << " "
                    << e.window.timestamp << " "
                    << e.window.windowID << " "
                    // Promote 8-bit integers (see notes above)
                    << +e.window.event << " "
                    << +e.window.padding1 << " "
                    << +e.window.padding2 << " "
                    << +e.window.padding3 << " "
                    << e.window.data1 << " "
                    << e.window.data2 << " "
                    << "#window event"
                    << (e.window.event == SDL_WINDOWEVENT_MAXIMIZED
                                          ? " (maximized)" : "")
                    << (e.window.event == SDL_WINDOWEVENT_MINIMIZED
                                          ? " (minimized)" : "")
                    << (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED
                                          ? " (size changed)" : "")
                    << (e.window.event == SDL_WINDOWEVENT_RESTORED
                                          ? " (restored)" : "")
                    << std::endl;
                break;
            case SDL_TEXTINPUT:
                m_recordingStream
                    << e.text.type << " "
                    << e.text.timestamp << " "
                    << e.text.windowID << " "
                    << "\"" << e.text.text << "\"" << " "
                    << "#text "
                    << e.text.text
                    << std::endl;
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                {
                m_recordingStream
                    << e.key.type << " "
                    << e.key.timestamp << " "
                    << e.key.windowID << " "
                    // Promote 8-bit integers (see notes above)
                    << +e.key.state << " "
                    << +e.key.repeat << " "
                    << +e.key.padding2 << " "
                    << +e.key.padding3 << " "
                    << e.key.keysym.scancode << " "
                    << e.key.keysym.sym << " "
                    << +e.key.keysym.mod << " "
                    << "#key "
                    << (e.key.state == SDL_PRESSED ? "pressed" : "released");
                std::string specialName = GetKeyNameIfSpecial(e.key.keysym.sym);
                if (specialName == "") {
                    m_recordingStream << " '" << char(e.key.keysym.sym) << "'";
                } else {
                    m_recordingStream << specialName;
                }
                m_recordingStream << std::endl;
                break;
                }
            case SDL_MOUSEMOTION:
                m_recordingStream
                    << e.motion.type << " "
                    << e.motion.timestamp << " "
                    << e.motion.windowID << " "
                    << e.motion.which << " "
                    << e.motion.state << " "
                    << e.motion.x << " "
                    << e.motion.y << " "
                    << e.motion.xrel << " "
                    << e.motion.yrel << " "
                    << "#motion ("
                    << (e.motion.state == SDL_PRESSED ? "pressed" : "released")
                    << ")"
                    << std::endl;
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                m_recordingStream
                    << e.button.type << " "
                    << e.button.timestamp << " "
                    << e.button.windowID << " "
                    << e.button.which << " "
                    // Promote 8-bit integers (see notes above)
                    << +e.button.button << " "
                    << +e.button.state << " "
                    << +e.button.clicks << " "
                    << +e.button.padding1 << " "
                    << e.button.x << " "
                    << e.button.y << " "
                    << "#button "
                    << (e.button.state == SDL_PRESSED ? "PRESSED" : "RELEASED")
                    << std::endl;
                break;
            case SDL_MOUSEWHEEL:
                m_recordingStream
                    << e.wheel.type << " "
                    << e.wheel.timestamp << " "
                    << e.wheel.windowID << " "
                    << e.wheel.which << " "
                    << e.wheel.x << " "
                    << e.wheel.y << " "
                    << e.wheel.direction << " "
                    << "#wheel"
                    << std::endl;
                break;
            case SDL_CONTROLLERAXISMOTION:
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED:
            case SDL_CONTROLLERDEVICEREMAPPED:
                m_recordingStream
                    << "#Controller event NOT RECORDED"
                    << std::endl;
                LOG_ERROR("Controller 'record event' called but not processed");
                break;
            case SDL_JOYAXISMOTION:
            case SDL_JOYBALLMOTION:
            case SDL_JOYHATMOTION:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                m_recordingStream
                    << "#Joystick event NOT RECORDED"
                    << std::endl;
                LOG_ERROR("Joystick 'record event' called but not processed");
                break;
            case SDL_FINGERDOWN:
            case SDL_FINGERUP:
            case SDL_FINGERMOTION:
                m_recordingStream
                    << e.tfinger.type << " "
                    << e.tfinger.timestamp << " "
                    << e.tfinger.touchId << " "
                    << e.tfinger.fingerId << " "
                    << e.tfinger.x << " "
                    << e.tfinger.y << " "
                    << e.tfinger.dx << " "
                    << e.tfinger.dy << " "
                    << e.tfinger.pressure << " "
//                    << e.tfinger.windowID << " "
                    << "#tfinger"
                    << std::endl;
                break;
            default:
                // Record all events (even empty and "non-registered" events)
                m_recordingStream
                    << "#no event"
                    << std::endl;
                break;
        }
    }

    SDL_Event SDLRenderer::getNextRecordedEvent(bool& endOfFile) {
        SDL_Event e;
        e.type = 0;
        endOfFile = false;
        int nextFrame;
        std::string eventLineString;
        std::getline(m_playbackStream, eventLineString);
        std::istringstream eventLine(eventLineString);
        if (!m_playbackStream || !eventLine) {
            if (m_playbackStream.eof()) {
                endOfFile = true;
            } else {
                LOG_FATAL("Error getting line from m_playbackStream");
            }
            return e;
        }
        eventLine >> nextFrame;
        if (!eventLine) {
            LOG_FATAL("Error getting frame number from m_playbackStream");
            return e;
        }
        eventLine >> e.type;
        if (!eventLine) {
            // No event associated with frame
            SDL_Event emptyEvent;
            emptyEvent.type = 0;
            return emptyEvent;
        }
        std::string textInput;
        switch (e.type) {
            case SDL_QUIT:
                eventLine
                    >> e.quit.timestamp;
                break;
            case SDL_WINDOWEVENT:
                int event, winPadding1, winPadding2, winPadding3;
                eventLine
                    >> e.window.timestamp
                    >> e.window.windowID
                    >> event
                    >> winPadding1
                    >> winPadding2
                    >> winPadding3
                    >> e.window.data1
                    >> e.window.data2;
                // stringstream will not correctly read into 8-bit integers and
                // some SDL types. Hence, read ints and cast to the variables
                e.window.event = static_cast<Uint8>(event);
                e.window.padding1 = static_cast<Uint8>(winPadding1);
                e.window.padding2 = static_cast<Uint8>(winPadding2);
                e.window.padding3 = static_cast<Uint8>(winPadding3);
                break;
            case SDL_TEXTINPUT:
                eventLine
                    >> e.text.timestamp
                    >> e.text.windowID
                    >> std::quoted(textInput);
                if (textInput.length() <= SDL_TEXTINPUTEVENT_TEXT_SIZE) {
                    strcpy(e.text.text, textInput.c_str());
                } else {
                    LOG_FATAL("Playback stream text too long for SDL");
                }
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                int keyState, repeat, padding2, padding3, scancode, sym, mod;
                eventLine
                    >> e.key.timestamp
                    >> e.key.windowID
                    >> keyState
                    >> repeat
                    >> padding2
                    >> padding3
                    >> scancode
                    >> sym
                    >> mod;
                // 8-bit variables need to be explicitly cast (see note above)
                e.key.state = static_cast<Uint8>(keyState);
                e.key.repeat = static_cast<Uint8>(repeat);
                e.key.padding2 = static_cast<Uint8>(padding2);
                e.key.padding3 = static_cast<Uint8>(padding3);
                e.key.keysym.scancode = static_cast<SDL_Scancode>(scancode);
                e.key.keysym.sym = static_cast<SDL_Keycode>(sym);
                e.key.keysym.mod = static_cast<Uint16>(mod);
                break;
            case SDL_MOUSEMOTION:
                eventLine
                    >> e.motion.timestamp
                    >> e.motion.windowID
                    >> e.motion.which
                    >> e.motion.state
                    >> e.motion.x
                    >> e.motion.y
                    >> e.motion.xrel
                    >> e.motion.yrel;
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                int button, buttonState, clicks, padding1;
                eventLine
                    >> e.button.timestamp
                    >> e.button.windowID
                    >> e.button.which
                    >> button
                    >> buttonState
                    >> clicks
                    >> padding1
                    >> e.button.x
                    >> e.button.y;
                // 8-bit variables need to be explicitly cast (see note above)
                e.button.button = static_cast<Uint8>(button);
                e.button.state = static_cast<Uint8>(buttonState);
                e.button.clicks = static_cast<Uint8>(clicks);
                e.button.padding1 = static_cast<Uint8>(padding1);
                break;
            case SDL_MOUSEWHEEL:
                eventLine
                    >> e.wheel.timestamp
                    >> e.wheel.windowID
                    >> e.wheel.which
                    >> e.wheel.x
                    >> e.wheel.y
                    >> e.wheel.direction;
                break;
            case SDL_CONTROLLERAXISMOTION:
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED:
            case SDL_CONTROLLERDEVICEREMAPPED:
                LOG_ERROR("Controller event in m_playbackStream not processed");
                break;
            case SDL_JOYAXISMOTION:
            case SDL_JOYBALLMOTION:
            case SDL_JOYHATMOTION:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                LOG_ERROR("Joystick event in m_playbackStream not processed");
                break;
            case SDL_FINGERDOWN:
            case SDL_FINGERUP:
            case SDL_FINGERMOTION:
                eventLine
                    >> e.tfinger.timestamp
                    >> e.tfinger.touchId
                    >> e.tfinger.fingerId
                    >> e.tfinger.x
                    >> e.tfinger.y
                    >> e.tfinger.dx
                    >> e.tfinger.dy
                    >> e.tfinger.pressure
//                    >> e.tfinger.windowID
                    ;
                break;
            default:
                std::ostringstream error;
                error << "Encountered unknown event in m_playbackStream at frame "
                      << nextFrame;
                LOG_FATAL(error.str().c_str());
                break;
        }
        if (!eventLine) {
            // An error occurred after reading the event type, during processing
            // of the event. Because event playback is intended to be a developer
            // feature for testing, minimal time has been invested in productizing
            // error checking (for event playback). If this error is tripped,
            // then diagnostics should be added to the event processing above.
            LOG_FATAL("Error reading event from m_playbackStream");
        }
        return e;
    }

    void SDLRenderer::setPauseRecordingEvents(const bool pause) {
        m_pauseRecordingOfEvents = pause;
    }

    bool SDLRenderer::stopRecordingEvents(std::string * errorMessage,
                                          const bool& error) {
        if (!m_recordingEvents) return true;

        bool success = true;
        if (!processKeyPressedAndMouseDownEvents(errorMessage)) {
            if (errorMessage != nullptr) {
                std::stringstream errorStream;
                errorStream << "While recording events to a file: " << errorMessage;
                *errorMessage = errorStream.str();
            }
            success = false; // Do not return -- write file even if flush error 
        }
        std::ofstream outFile(m_recordingFileName, std::ios::out);
        if(outFile) {
            if (!m_playingBackEvents) { // normal recording
                // Write out file header
                outFile << "# File containing settings.json, imgui.ini, and"
                        << " recorded SDL events for playback" << std::endl;
                outFile << "#" << std::endl;
                if (m_jsonSettings != "") outFile << m_jsonSettings;
                if (m_imGuiIniFileSize > 0 && m_imGuiIniFileCharPtr != nullptr) {
                    outFile << "# imgui.ini size:" << std::endl;
                    outFile << m_imGuiIniFileSize << std::endl;
                    outFile << "# Begin imgui.ini file:" << std::endl;
                    outFile << m_imGuiIniFileCharPtr;
                } else {
                    // Use same formatting as above for error checking
                    outFile << "# No imgui.ini file loaded, using default window "
                            << "placement -- imgui.ini size:" << std::endl;
                    outFile << 0 << std::endl;
                    outFile << "#" << std::endl;
                }
            } else { // m_playingBackEvents
                outFile << m_eventsFileHeaderStream.str();
            }
            outFile << "# Recorded SDL events:" << std::endl
                    << "# Format: frame_number"
                    << " event_data #comment" << std::endl;
            if (!m_playingBackEvents) {
                // Write initial event to place mouse in window
                outFile << "0 1024 0 2 0 0 100 100 0 0 #motion (released)\n";
            }
            // Write out recorded events
            outFile << m_recordingStream.str();
            // Close file and clear stream
            outFile.close();

            // Copy events file to default location and archive (if requested)
            Log::CopyFileOrWriteLogIfError(m_recordingFileName, Log::GetEventsPath());
            if (m_autoRecordEvents && (m_recordingFileName != Log::GetEventsArchivePath())) {
                Log::CopyFileOrWriteLogIfError(m_recordingFileName, Log::GetEventsArchivePath());
            }

            if (error) {
                // Add error label to event files
                Log::AppendLabelToFileStemOrWriteLogIfError(Log::GetEventsPath(), "_ERROR");
                if (m_autoRecordEvents) {
                    Log::AppendLabelToFileStemOrWriteLogIfError(Log::GetEventsArchivePath(), "_ERROR");
                }
            }

            std::stringstream().swap(m_recordingStream);
        } else {
            if (errorMessage != nullptr) {
                std::stringstream errorStream;
                if (!success) {
                    errorStream << "1st error message: " << errorMessage
                                << "\n 2nd error message: ";
                }
                errorStream  << "File '" << m_recordingFileName
                    << "' could not be opened to writing events to playback file."
                    << std::endl;
                *errorMessage = errorStream.str();
            }
            success = false;
        }
        m_recordingEvents = false;
        return success;
    }

    bool SDLRenderer::recordingEvents() {
        return m_recordingEvents;
    }

    void SDLRenderer::startPlayingEvents() {
        m_playingBackEvents = true;
        ResetMouseMotionLoggingForPlayback();
    }

    void SDLRenderer::ResetMouseMotionLoggingForPlayback() {
        m_loggedUserMousePosInPlayback = false;
        m_numTimesMaxMouseMotionExeededForPlayback = 0;
    }

    void SDLRenderer::ManageMouseMotionLoggingForPlayback() {
        if (m_numTimesMaxMouseMotionExeededForPlayback > 0) {
            if ((frameNumber - m_lastFrameMouseMotionExceededForPlayback) > 10) {
                // Throw out singular large mouse motion events
                ResetMouseMotionLoggingForPlayback();
            }
        }
    }

    bool SDLRenderer::readRecordedEvents(const std::string& fileName,
                                         std::string& errorMessage) {
        bool success = true;

        std::ifstream inFile(fileName, std::ios::in);
        if(inFile) {
            // Read the first group of commented lines and skip settings.json
            std::string fileLineString;
            bool lineStartsWithHash = true;
            std::getline(inFile, fileLineString);
            while (inFile && lineStartsWithHash) {
                if (m_recordingEventsRequested) {
                    m_eventsFileHeaderStream << fileLineString << std::endl;
                }
                std::stringstream settingsStream;
                if (fileLineString[0] != '#') lineStartsWithHash = false;
                settingsStream = GetSettingsAndAdvanceEventsStreamIfAble(
                                                         fileLineString, &inFile);
                if (m_recordingEventsRequested) {
                    m_eventsFileHeaderStream << settingsStream.str();
                }
                if (lineStartsWithHash) std::getline(inFile, fileLineString);
            }

            // Read Imgui character stream size
            char c;
            size_t imGuiSize = 0;
            bool endOfFile = false;
            std::istringstream fileLine(fileLineString);
            if (!inFile || !fileLine) {
                if (inFile.eof()) {
                    endOfFile = true;
                    errorMessage = "Events playback file is empty or could not find end of settings.json section";
                } else {
                    errorMessage = "Error in events playback file format: error reading first line in expected imgui.ini section";
                }
                return false;
            }
            fileLine >> imGuiSize;
            if (!fileLine) {
                errorMessage = "Error in events playback file format: cannot read imgui.ini file size in expected imgui.ini section";
                return false;
            }
            if (imGuiSize > 0) {
                // Check that a '#' has been placed after imgui.ini file size
                std::getline(inFile, fileLineString);
                if (m_recordingEventsRequested) {
                    m_eventsFileHeaderStream << fileLineString << std::endl;
                }
                if (!inFile || fileLineString[0] != '#') {
                    errorMessage = "Expected '#' after reading imgui.ini file size from events playback file";
                    return false;
                }
                // Read the imgui.ini character stream
                bool foundHash = false;
                std::stringstream imGuiStream;
                for (int i = 0; i < imGuiSize; i++) {
                    if (inFile.get(c)) {
                        if (c == '#') {
                            char tempChar;
                            if (!inFile.get(tempChar)) {
                                errorMessage = "Could not read next character of imgui.ini file from events playback file";
                                return false;
                            }
                            if (tempChar == ' ' || i == imGuiSize-1) {
                                std::ostringstream error;
                                error << "Reached end of imgui.ini file after "
                                      << i << " characters, but events file"
                                      << " specified size of imgui.ini to be "
                                      << imGuiSize << " characters. Please adjust"
                                      << " the size specification for imgui.ini in"
                                      << " the events file.";
                                errorMessage = error.str();
                                return false;
                            }
                            inFile.unget();
                        }
                        imGuiStream << c;
                    } else {
                        errorMessage = "Error reading imgui.ini file from events playback file";
                        return false;
                    }
                }
                if (!inFile.get(c)) {
                    errorMessage = "Could not read character after imgui.ini file from events playback file";
                    return false;
                }
                if (c != '#') {
                    std::ostringstream next22Chars;
                    next22Chars << c;
                    for (int i = 0; i < 22; i++) {
                        if (!inFile.get(c)) {
                            errorMessage = "Could not read next 22 characters after imgui.ini file from events playback file";
                            return false;
                        }
                        next22Chars << c;
                    }
                    std::ostringstream error;
                    error << "The next 22 characters after reading the specified "
                          << imGuiSize << " characters for the the imgui.ini file"
                          << " are (including newlines): " << std::endl << std::endl
                          << next22Chars.str() << std::endl << std::endl;
                    error << "The next character after the imgui.ini file must"
                          << " start with '#'. Try substantially increasing the"
                          << " specified size of the imgui.ini file -- the parser"
                          << " will then tell you the correct size.";
                    errorMessage = error.str();
                    return false;
                }
                inFile.unget();
                // The c_str from std::stringstream deallocates after statement
                // So, store in a const temporary that will exist while in scope
                const std::string& imGuiString = imGuiStream.str();
                // Load the imgui.ini character stream into ImGui
                ImGui::LoadIniSettingsFromMemory(imGuiString.c_str(), imGuiSize);
                if (m_recordingEventsRequested) {
                    m_eventsFileHeaderStream << imGuiStream.str();
                }
            }

            // Read the rest of file into the 'm_playbackStream' member variable
            std::stringstream().swap(m_playbackStream);
            while (std::getline(inFile, fileLineString)) {
                if (fileLineString[0] != '#') { // Ignore commented lines
                    m_playbackStream << fileLineString << std::endl;
                }
            }
            inFile.close();
        } else {
            std::stringstream errorStream;
            errorStream << "File '" << fileName << "' could not be opened for events playback."
                        << std::endl;
            errorMessage = errorStream.str();
            success = false;
        }
        return success;
    }

    std::stringstream
    SDLRenderer::GetSettingsFromEventsFile(const std::string& fileName) {
        std::ifstream inFile(fileName, std::ios::in);
        if(inFile) {
            std::stringstream settingsStream;
            std::string fileLineString;
            std::getline(inFile, fileLineString);
            while (inFile) {
                settingsStream = GetSettingsAndAdvanceEventsStreamIfAble(
                                                         fileLineString, &inFile);
                if (settingsStream.str() != "") {
                    inFile.close();
                    return settingsStream;
                } else {
                    std::getline(inFile, fileLineString);
                }
            }
        } else {
            LOG_ERROR("Could not open events file in 'GetSettingsFromEventsFile.");
        }
        inFile.close();
        return std::stringstream();
    }

    std::stringstream
    SDLRenderer::GetSettingsAndAdvanceEventsStreamIfAble(
                                                   const std::string& currentLine,
                                                   std::ifstream* eventsFilePtr) {
        std::string jsonHeader("# Begin settings.json file:");
        if (currentLine.substr(0,27) != jsonHeader.substr(0,27)) {
            return std::stringstream(); // Return blank stringstream
        }
        std::stringstream settingsStream;
        std::string jsonFooter("# End settings.json file");
        std::ifstream& eventsFile = *eventsFilePtr;
        std::string nextLine;
        while (std::getline(eventsFile, nextLine)) {
            if (nextLine.substr(0,24) == jsonFooter.substr(0,24)) {
                // Unget footer so that it is included in m_eventsFileHeaderStream
                // nextLine.size() does not include the std::endl, so unget size+1
                for (int i = nextLine.size(); i >= 0; --i) eventsFile.unget();
                return settingsStream;
            }
            settingsStream << nextLine << std::endl;
        }
        return std::stringstream(); // Return blank stringstream
    }

    void SDLRenderer::setPausePlayingEvents(const bool pause) {
        m_pausePlaybackOfEvents = pause;
    }

    bool SDLRenderer::playingEvents() {
        return m_playingBackEvents;
    }

    bool SDLRenderer::playingEventsAborted() {
        return m_playingBackEventsAborted;
    }

    std::vector<SDL_Event> SDLRenderer::getRecordedEventsForNextFrame() {
        // Return events in m_playbackStream associated with next frame
        std::vector<SDL_Event> events;
        bool success = true;
        bool endOfFile = false;
        int nextFrame = nextRecordedFramePeek();
        m_playbackFrame = nextFrame;
        while (nextFrame == m_playbackFrame && !endOfFile) {
            events.push_back(getNextRecordedEvent(endOfFile));
            nextFrame = nextRecordedFramePeek();
        }
        if (endOfFile) {
            m_playingBackEvents = false;
        } else {
            m_playbackFrame = nextFrame;
        }
        return events;
    }

    int SDLRenderer::nextRecordedFramePeek() {
        char c;
        bool getNextChar = true;
        std::vector<char> digits;
        while (m_playbackStream && getNextChar && m_playbackStream.get(c)) {
            if (isdigit(c)) {
                digits.push_back(c);
            } else {
                getNextChar = false;
                m_playbackStream.unget();
            }
        }
        std::stringstream numberString;
        for (int i = 0; i < digits.size(); i++) {
            numberString << digits[i];
            m_playbackStream.unget();
        }
        int nextFrame;
        if (!(numberString >> nextFrame)) {
            nextFrame = -99;
        }
        return nextFrame;
    }

    void SDLRenderer::captureFrameToFile(RenderPass * renderPass,
                                           std::filesystem::path path,
                                           bool captureFromScreen) {
        std::cout << "Writing single image to filesystem..." << std::endl;
        glm::ivec2 dimensions = renderPass->frameSize();
        writeImage(renderPass->readRawPixels(0, 0, dimensions.x,
                   dimensions.y, captureFromScreen), dimensions, path);
    }

    void SDLRenderer::captureFrame(RenderPass * renderPass, bool captureFromScreen) {
        int i = m_image.size();
        m_imageDimensions.push_back(renderPass->frameSize());
        m_image.push_back(renderPass->readRawPixels(0, 0, m_imageDimensions[i].x,
                                       m_imageDimensions[i].y, captureFromScreen));
    }

    int SDLRenderer::numCapturedImages() {
        return m_image.size();
    }

    void SDLRenderer::writeCapturedImages(std::string fileName) {
        if (m_writingImages) {
            return;
        }
        m_writingImages = true;
   
        LOG_ASSERT(m_image.size() == m_imageDimensions.size());
        if (m_image.size() > 0) {
            std::cout << "Writing images to filesystem..." << std::endl;
        }
        for (int i = 0; i < m_image.size(); i++) {
            drawFrame(); // Keep ImGui drawing updated during write of images

            std::stringstream imageFileName;
            imageFileName << fileName << i+1 << ".png"; // Start numbering at 1
            std::filesystem::path path = imageFileName.str();
            writeImage(m_image[i], m_imageDimensions[i], path);
        }

        m_writingImages = false;
    }

    void SDLRenderer::writeImage(std::vector<glm::u8vec4> image,
                                 glm::ivec2 imageDimensions,
                                 std::filesystem::path path) {
        stbi_flip_vertically_on_write(true);
        int stride = Color::numChannels() * imageDimensions.x;
        stbi_write_png(path.string().c_str(),
                       imageDimensions.x, imageDimensions.y,
                       Color::numChannels(), image.data(), stride);
    }

    void SDLRenderer::addCommentToEventsFile(std::string_view comment) {
        if (!m_recordingEvents) return;
        m_recordingStream << "# " << comment << std::endl;
    }

    void SDLRenderer::addKeyPressed(SDL_Keycode keyCode) {
        if (!isKeyPressed(keyCode)) {
            keyPressed.push_back(keyCode);
        }
    }

    void SDLRenderer::removeKeyPressed(SDL_Keycode keyCode) {
        auto &v = keyPressed;
        // Use the the "erase-remove idiom" enabled by std::algorithm
        v.erase(std::remove(v.begin(), v.end(), keyCode), v.end());
    }

    bool SDLRenderer::isKeyPressed(SDL_Keycode keyCode) {
        for (auto key : keyPressed) {
            if (key == keyCode) {
                return true;
            }
        }
        return false;
    }

    bool SDLRenderer::isAnyKeyPressed() {
        if (keyPressed.size() > 0) {
            return true;
        }
        return false;
    }

    SDLRenderer::InitBuilder::~InitBuilder() {
        build();
    }

    SDLRenderer::InitBuilder::InitBuilder(SDLRenderer *sdlRenderer)
            :sdlRenderer(sdlRenderer) {
    }

    SDLRenderer::InitBuilder &SDLRenderer::InitBuilder::withSdlInitFlags(uint32_t sdlInitFlag) {
        this->sdlInitFlag = sdlInitFlag;
        return *this;
    }

    SDLRenderer::InitBuilder &SDLRenderer::InitBuilder::withSdlWindowFlags(uint32_t sdlWindowFlags) {
        this->sdlWindowFlags = sdlWindowFlags;
        return *this;
    }

    SDLRenderer::InitBuilder &SDLRenderer::InitBuilder::withDpiAwareness(bool isDpiAware) {
        this->isDpiAware = isDpiAware;
        return *this;
    }

    SDLRenderer::InitBuilder &SDLRenderer::InitBuilder::withVSync(bool vsync) {
        this->vsync = vsync;
        return *this;
    }

    SDLRenderer::InitBuilder &SDLRenderer::InitBuilder::withGLVersion(int majorVersion, int minorVersion) {
        this->glMajorVersion = majorVersion;
        this->glMinorVersion = minorVersion;
        return *this;
    }

    bool SDLRenderer::UsingOpenGL_EGL() {
#ifdef OPENGL_EGL
        return true;
#else
        return false;
#endif
    }

    void SDLRenderer::InitBuilder::build() {
        if (sdlRenderer->running){
            return;
        }
        if (!sdlRenderer->window){
#ifdef EMSCRIPTEN
            SDL_Renderer *renderer = nullptr;
            SDL_CreateWindowAndRenderer(sdlRenderer->windowWidth, sdlRenderer->windowHeight, SDL_WINDOW_OPENGL, &sdlRenderer->window, &renderer);
#else
            if (isDpiAware) SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1"); // TODO: (#19) Replace with appropriate SDL3 call
            SDL_Init(0);
            if (SDL_VideoInit("wayland") == 0 && UsingOpenGL_EGL()) {
                SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland");
            }
            SDL_Init( sdlInitFlag  );
            SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, glMajorVersion);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, glMinorVersion);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef SRE_DEBUG_CONTEXT
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
            sdlRenderer->window = SDL_CreateWindow(sdlRenderer->windowTitle.c_str(), sdlRenderer->windowPosition.x, sdlRenderer->windowPosition.y, sdlRenderer->windowWidth, sdlRenderer->windowHeight,sdlWindowFlags);
#endif
            sdlRenderer->r = new Renderer(sdlRenderer->window, vsync, maxSceneLights);

            sdlRenderer->SetMinimalRendering(minimalRendering);
            sdlRenderer->isWindowHidden = (SDL_GetWindowFlags(sdlRenderer->window) & SDL_WINDOW_HIDDEN);

#ifdef SRE_DEBUG_CONTEXT
            if (glDebugMessageCallback) {
                LOG_INFO("Register OpenGL debug callback ");

                std::cout << "Register OpenGL debug callback " << std::endl;
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(openglCallbackFunction, nullptr);
                GLuint unusedIds = 0;
                glDebugMessageControl(GL_DONT_CARE,
                    GL_DONT_CARE,
                    GL_DONT_CARE,
                    0,
                    &unusedIds,
                    true);

            }
#endif
        }
    }

    SDLRenderer::InitBuilder &SDLRenderer::InitBuilder::withMaxSceneLights(int maxSceneLights) {
        this->maxSceneLights = maxSceneLights;
        return *this;
    }

    SDLRenderer::InitBuilder &SDLRenderer::InitBuilder::withMinimalRendering(bool minimalRendering) {
        this->minimalRendering = minimalRendering;
        return *this;
    }

    void SDLRenderer::setWindowIcon(std::shared_ptr<Texture> tex){
        auto texRaw = tex->getRawImage();
        auto surface = SDL_CreateRGBSurfaceFrom(texRaw.data(),tex->getWidth(),tex->getHeight(),32,tex->getWidth()*4,0x00ff0000,0x0000ff00,0x000000ff,0xff000000);

        // The icon is attached to the window pointer
        SDL_SetWindowIcon(window, surface);

        // ...and the surface containing the icon pixel data is no longer required.
        SDL_FreeSurface(surface);

    }
}
