/*
 *  SimpleRenderEngine (https://github.com/mortennobel/SimpleRenderEngine)
 *
 *  Created by Morten Nobel-Jørgensen ( http://www.nobel-joergensen.com/ )
 *  License: MIT
 */

#pragma once

#include "sre/Camera.hpp"
#include "sre/Color.hpp"
#include "sre/Mesh.hpp"
#include "sre/Material.hpp"
#include "sre/WorldLights.hpp"
#include <string>
#include <functional>

#include "sre/impl/Export.hpp"
#include "SpriteBatch.hpp"
#include "Skybox.hpp"

namespace sre {
    class Renderer;

    class Shader;
    class Material;
    class RenderStats;
    class Framebuffer;

    // A render pass encapsulates some render states and allows adding draw-calls.
    // Materials and shaders are assumed not to be modified during a renderpass.
    // Note that only one render pass object can be active at a time.
    class DllExport RenderPass {
    public:
        class DllExport RenderPassBuilder {
        public:
            RenderPassBuilder& withName(const std::string& name);
            RenderPassBuilder& withCamera(const Camera& camera);
            RenderPassBuilder& withWorldLights(WorldLights* worldLights);

            RenderPassBuilder& withClearColor(bool enabled = true,Color color = {0,0,0,1});    // Set the clear color.
                                                                                                   // Default enabled with the color value {0.0,0.0,0.0,1.0}

            RenderPassBuilder& withSkybox(std::shared_ptr<Skybox> skybox);                     // Set clear color to skybox.

            RenderPassBuilder& withClearDepth(bool enabled = true, float value = 1);               // Set the clear depth. Value is clamped between [0.0;1.0]
                                                                                                   // Default: enabled with depth value 1.0

            RenderPassBuilder& withClearStencil(bool enabled = false, int value = 0);              // Set the clear depth. Value is clamped between
                                                                                                   // Default: disabled

            RenderPassBuilder& withGUI(bool enabled = true);                                       // Allows ImGui calls to be called in the renderpass and
                                                                                                   // calls ImGui::Render() in the end of the renderpass

            RenderPassBuilder& withFramebuffer(std::shared_ptr<Framebuffer> framebuffer);
            RenderPass build();
        private:
            RenderPassBuilder() = default;
            // prevent creating instances of this object
            // (note it is still possible to keep a universal reference)
            RenderPassBuilder(const RenderPassBuilder& r) = default;
            std::string name;
            std::shared_ptr<Framebuffer> framebuffer;
            WorldLights* worldLights = nullptr;
            Camera camera;
            RenderStats* renderStats = nullptr;

            bool clearColor = true;
            glm::vec4 clearColorValue = {0,0,0,1};
            bool clearDepth = true;
            float clearDepthValue = 1.0f;
            bool clearStencil = false;
            int clearStencilValue = 0;
            std::shared_ptr<Skybox> skybox;

            bool gui = true;

            explicit RenderPassBuilder(RenderStats* renderStats);
            friend class RenderPass;
            friend class Renderer;
            friend class Inspector;
        };

        static RenderPassBuilder create();   // Create a RenderPass

        RenderPass(RenderPass&& rp) noexcept;
        RenderPass& operator=(RenderPass&& other) = delete;             // RenderPass objects cannot be reused.
        virtual ~RenderPass();

        void drawLines(const std::vector<glm::vec3> &verts,             // Draws worldspace lines.
                       Color color = {1.0f, 1.0f, 1.0f, 1.0f},          // Note that this member function is not expected
                       MeshTopology meshTopology = MeshTopology::Lines);// to perform as efficient as draw()

        void draw(std::shared_ptr<Mesh>& mesh,                          // Draws a mesh using the given transform and material.
                  glm::mat4 modelTransform,                             // The modelTransform defines the modelToWorld
                  std::shared_ptr<Material>& material);                 // transformation.

        void draw(std::shared_ptr<Mesh>& mesh,                          // Draws a mesh using the given transform and materials.
                  glm::mat4 modelTransform,                             // The modelTransform defines the modelToWorld transformation
                  std::vector<std::shared_ptr<Material>> materials);    // The number of materials must match the size of index sets in the model

        void draw(std::shared_ptr<SpriteBatch>& spriteBatch,            // Draws a spriteBatch using modelTransform
                  glm::mat4 modelTransform = glm::mat4(1));             // using a model-to-world transformation

        void draw(std::shared_ptr<SpriteBatch>&& spriteBatch,           // Draws a spriteBatch using modelTransform
                  glm::mat4 modelTransform = glm::mat4(1));             // using a model-to-world transformation

        void blit(std::shared_ptr<Texture> texture,                     // Render texture to screen
                  glm::mat4 transformation = glm::mat4(1.0f));

        void blit(std::shared_ptr<Material> material,                   // Render material to screen
                  glm::mat4 transformation = glm::mat4(1.0f));

        glm::vec2 frameSize();                                          // Return the frame/window size for this RenderPass 

        std::vector<Color> readPixels(unsigned int x,                   // Reads pixel(s) from the current framebuffer and returns an array of color values
                                          unsigned int y,               // The defined rectangle must be within the size of the current framebuffer
                                          unsigned int width = 1,       // This function must be called after finish has been explicitly called on the renderPass
                                          unsigned int height = 1,      // If readFromScreen == true, then read from the screen instead of the framebuffer, which
                                          bool readFromScreen = false); // is useful when a multipass framebuffer is attached (in this case glReadPixels won't work)

        std::vector<glm::u8vec4> readRawPixels(unsigned int x,          // Similar to 'readPixels' above, except this returns array of 8-bit values
                                          unsigned int y,               // (the format is RGBA per pixel: each pixel generates 4 x 8-bits = 32 bits = 4 bytes,
                                          unsigned int width = 1,       // or one item in the u8vec4 array), useful for using e.g. stbi_write to write images
                                          unsigned int height = 1,
                                          bool readFromScreen = false);

        void finishGPUCommandBuffer();                                  // GPU command buffer (must be called when
                                                                        // profiling GPU time - should not be called
                                                                        // when not profiling)

        void finish();
        bool isFinished();
    private:
        RenderPass(const RenderPass&) = default;
        struct FrameInspector {
            int frameid = -1;
            std::vector<std::shared_ptr<RenderPass>> renderPasses;
        };

        static FrameInspector frameInspector;

        bool mIsFinished = false;
        struct RenderQueueObj{
            std::shared_ptr<Mesh> mesh;
            glm::mat4 modelTransform;
            std::shared_ptr<Material> material;
            int subMesh = 0;
        };
        struct GlobalUniforms{
            glm::mat4* g_view;
            glm::mat4* g_projection;
            glm::vec4* g_viewport;
            glm::vec4* g_cameraPos;
            glm::vec4* g_ambientLight;
            glm::vec4* g_lightColorRange;
            glm::vec4* g_lightPosType;
        };
        std::vector<RenderQueueObj> renderQueue;

        void drawInstance(RenderQueueObj& rqObj);                       // perform the actual rendering

        RenderPass::RenderPassBuilder builder;
        explicit RenderPass(RenderPass::RenderPassBuilder& builder);

        void setupShaderRenderPass(Shader *shader);
        void setupShaderRenderPass(const GlobalUniforms& globalUniforms);
        void setupGlobalShaderUniforms();
        void setupShader(const glm::mat4 &modelTransform, Shader *shader);

        Shader* lastBoundShader = nullptr;
        Material* lastBoundMaterial = nullptr;
        int64_t lastBoundMeshId = -1;

        glm::mat4 projection;
        glm::uvec2 viewportOffset;
        glm::uvec2 viewportSize;

        friend class Renderer;
        friend class Inspector;
    };
}
