/*
 *  SimpleRenderEngine (https://github.com/mortennobel/SimpleRenderEngine)
 *
 *  Created by Morten Nobel-Jørgensen ( http://www.nobel-joergensen.com/ )
 *  License: MIT
 */

#pragma once

#include "glm/glm.hpp"
#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <map>
#include "sre/MeshTopology.hpp"

#include "sre/impl/Export.hpp"
#include "Shader.hpp"
#include "RenderStats.hpp"


namespace sre {
    // forward declaration
    class Shader;
    class Inspector;
    class RenderPass;

    /**
     * Represents a Mesh object.
     * A mesh is composed of a list of named vertex attributes such as
     * - position (vec3)
     * - normal (vec3)
     * - tangent (vec3)
     * - uv (aka. texture coordinates) (vec4)
     * - color (vec4)
     *
     * A mesh also has a meshType, which can be either: MeshTopology::Points, MeshTopology::Lines, or MeshTopology::Triangles
     *
     * The number and types of vertex attributes cannot be changed after the mesh has been created. The number of
     * vertices is allow to change.
     *
     * Note that each mesh can have multiple index sets associated with it which allows for using multiple materials for rendering.
     */
    class DllExport Mesh : public std::enable_shared_from_this<Mesh> {
    public:
        class DllExport MeshBuilder {
        public:
            // primitives
            MeshBuilder& withSphere(int stacks = 16, int slices = 32, float radius = 1);        // Creates a sphere mesh including UV coordinates, positions and
                                                                                                // normals
            MeshBuilder& withCube(float length = 1);                                            // Creates a cube including UV coordinates, positions and normals
            MeshBuilder& withWireCube(float length = 1);                                        // Creates a wire-frame cube with positions
            MeshBuilder& withWirePlane(int numLines, float length = 1);                         // Creates a wire-frame grid plane with numLines 
            MeshBuilder& withQuad(float size=1);                                                // Creates a quad x,y = [-size;size] and z=0, UV=[0;1],
                                                                                                //                                               normals=(0,0,1)
            MeshBuilder& withTorus(int segmentsC = 24, int segmentsA = 24, float radiusC = 1, float radiusA = .25);
                                                                                                // Creates a torus in xy plane. C is in the outer (large) circle,
                                                                                                // A is the sweeping circle.
            
            // properties
            MeshBuilder& withLineWidth(float lineWidth);                                        // Width of lines
            MeshBuilder& withLocation(glm::vec3 location);                                      // World space location of the mesh for RenderPass (RP) draw
            MeshBuilder& withRotation(glm::vec3 rotation);                                      // World space x, y, z rotation in Euler angles for RP draw
            MeshBuilder& withScaling(glm::vec3 directionalScaling);                             // World space x, y, z scaling for RP draw
            MeshBuilder& withScaling(float scaling);                                            // World space uniform scaling for RP draw
            MeshBuilder& withMaterial(std::shared_ptr<Material> material);                      // Material for RenderPass draw 

            // raw data
            MeshBuilder& withPositions(const std::vector<glm::vec3> &vertexPositions);          // Set vertex attribute "position" of type vec3
            MeshBuilder& withNormals(const std::vector<glm::vec3> &normals);                    // Set vertex attribute "normal" of type vec3
            MeshBuilder& withUVs(const std::vector<glm::vec4> &uvs);                            // Set vertex attribute "uv" of type vec4 (treated as two sets of
                                                                                                // texture coordinates)
            MeshBuilder& withColors(const std::vector<glm::vec4> &colors);                      // Set vertex attribute "color" of type vec4
            MeshBuilder& withTangents(const std::vector<glm::vec4> &tangent);                   // Set vertex attribute "tangent" of type vec4
            MeshBuilder& withParticleSizes(const std::vector<float> &particleSize);             // Set vertex attribute "particleSize" of type float
            MeshBuilder& withMeshTopology(MeshTopology meshTopology);                           // Defines the meshTopology (default is Triangles)
            DEPRECATED("Use with withIndices(std::vector<uint32_t>, MeshTopology, int)")
            MeshBuilder& withIndices(const std::vector<uint16_t> &indices, MeshTopology meshTopology = MeshTopology::Triangles, int indexSet=0);
            MeshBuilder& withIndices(const std::vector<uint32_t> &indices, MeshTopology meshTopology = MeshTopology::Triangles, int indexSet=0);
                                                                                                // Defines the indices (if no indices defined then the vertices
                                                                                                // are rendered sequeantial)
            // custom data layout
            MeshBuilder& withAttribute(std::string name, const std::vector<float> &values);       // Set a named vertex attribute of float
            MeshBuilder& withAttribute(std::string name, const std::vector<glm::vec2> &values);   // Set a named vertex attribute of vec2
            MeshBuilder& withAttribute(std::string name, const std::vector<glm::vec3> &values);   // Set a named vertex attribute of vec3
            MeshBuilder& withAttribute(std::string name, const std::vector<glm::vec4> &values);   // Set a named vertex attribute of vec4
            MeshBuilder& withAttribute(std::string name, const std::vector<glm::i32vec4> &values);// Set a named vertex attribute of i32vec4. On platforms not
                                                                                                  // supporting i32vec4 the values are converted to vec4

            // other
            MeshBuilder& withName(const std::string& name);                                       // Defines the name of the mesh
            MeshBuilder& withRecomputeNormals(bool enabled);                                      // Recomputes normals using angle weighted normals
            MeshBuilder& withRecomputeTangents(bool enabled);                                     // Recomputes tangents using (Lengyel’s Method)
            
            std::shared_ptr<Mesh> build();
        private:
            std::vector<glm::vec3> computeNormals();
            std::vector<glm::vec4> computeTangents(const std::vector<glm::vec3>& normals);
            MeshBuilder() = default;
            MeshBuilder(const MeshBuilder&) = default;
            std::map<std::string,std::vector<float>> attributesFloat;
            std::map<std::string,std::vector<glm::vec2>> attributesVec2;
            std::map<std::string,std::vector<glm::vec3>> attributesVec3;
            std::map<std::string,std::vector<glm::vec4>> attributesVec4;
            std::map<std::string,std::vector<glm::i32vec4>> attributesIVec4;
            std::vector<MeshTopology> meshTopology = {MeshTopology::Triangles};
            std::vector<std::vector<uint32_t>> indices;
            Mesh *updateMesh = nullptr;
            bool recomputeNormals = false;
            bool recomputeTangents = false;
            std::string name;
            float lineWidth {1.0f};
            glm::vec3 location {0.0f, 0.0f, 0.0f};
            glm::vec3 rotation {0.0f, 0.0f, 0.0f};
            glm::vec3 scaling {1.0f, 1.0f, 1.0f};
            std::shared_ptr<Material> material {nullptr};
            friend class Mesh;
        };

        ~Mesh();

        static MeshBuilder create();                                // Create Mesh using the builder pattern. (Must end with build()).
        MeshBuilder update();                                       // Update the mesh using the builder pattern. (Must end with build()).

        int getVertexCount();                                       // Number of vertices in mesh

        std::vector<glm::vec3> getPositions();                      // Get position vertex attribute
        std::vector<glm::vec3> getNormals();                        // Get normal vertex attribute
        std::vector<glm::vec4> getUVs();                            // Get uv vertex attribute
        std::vector<glm::vec4> getColors();                         // Get color vertex attribute
        std::vector<glm::vec4> getTangents();                       // Get tangent vertex attribute (the w component contains the orientation of bitangent:
                                                                    //                                                                                  -1 or 1)
        std::vector<float> getParticleSizes();                      // Get particle size vertex attribute

        int getIndexSets();                                         // Return the number of index sets
        MeshTopology getMeshTopology(int indexSet=0);               // Mesh topology used
        const std::vector<uint32_t>& getIndices(int indexSet=0);    // Indices used in the mesh
        int getIndicesSize(int indexSet=0);                         // Return the size of the index set

        template<typename T>
        inline T get(std::string attributeName);                    // Get the vertex attribute of a given type. Type must be float,glm::vec2,
                                                                    //                                                          glm::vec3,glm::vec4,glm::i32vec4

        std::pair<int,int> getType(const std::string& name);        // return element type, element count

        std::vector<std::string> getAttributeNames();               // Names of the vertex attributes

        std::array<glm::vec3,2> getBoundsMinMax();                  // get the local axis aligned bounding box (AABB)
        void setBoundsMinMax(const std::array<glm::vec3,2>& minMax);// set the local axis aligned bounding box (AABB)

        const std::string& getName();                               // Return the mesh name

        int getDataSize();                                          // get size of the mesh in bytes on GPU

        glm::vec3 getLocation();                                    // Get the location of the mesh
        void setLocation(glm::vec3 newLocation);                    // Set the location of the mesh
        void setRotation(glm::vec3 newRotation);                    // Set the rotation of the mesh using x, y, z Euler angles
        void setScaling(glm::vec3 newDirectionalScaling);           // Scale the mesh in different amounts in x, y, and z directions
        void setScaling(float newScaling);                          // Scale the mesh in the same amount in all directions
        void setMaterial(std::shared_ptr<Material> newMaterial);    // Set the material for the mesh
        void draw(RenderPass& renderPass);                          // Draw the mesh using renderPass
    private:
        struct Attribute {
            int offset;
            int elementCount;
            int dataType;
            int attributeType; // GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT
            int enabledAttributes[10];
            int disabledAttributes[10];
        };
        struct ElementBufferData {
            uint32_t offset;
            uint32_t size;
            uint32_t type;
        };

        Mesh       (std::map<std::string,std::vector<float>>&& attributesFloat, std::map<std::string,std::vector<glm::vec2>>&& attributesVec2, std::map<std::string, std::vector<glm::vec3>>&& attributesVec3, std::map<std::string,std::vector<glm::vec4>>&& attributesVec4,std::map<std::string,std::vector<glm::i32vec4>>&& attributesIVec4, std::vector<std::vector<uint32_t>> &&indices, std::vector<MeshTopology> meshTopology, std::string name, RenderStats& renderStats, float lineWidth, glm::vec3 location, glm::vec3 rotation, glm::vec3 scaling, std::shared_ptr<Material> material);
        void update(std::map<std::string,std::vector<float>>&& attributesFloat, std::map<std::string,std::vector<glm::vec2>>&& attributesVec2, std::map<std::string, std::vector<glm::vec3>>&& attributesVec3, std::map<std::string,std::vector<glm::vec4>>&& attributesVec4,std::map<std::string,std::vector<glm::i32vec4>>&& attributesIVec4, std::vector<std::vector<uint32_t>> &&indices, std::vector<MeshTopology> meshTopology, std::string name, RenderStats& renderStats, float lineWidth, glm::vec3 location, glm::vec3 rotation, glm::vec3 scaling, std::shared_ptr<Material> material);

        void updateIndexBuffers();
        std::vector<float> getInterleavedData();

        int totalBytesPerVertex = 0;
        static uint16_t meshIdCount;
        uint16_t meshId;

        void setVertexAttributePointers(Shader* shader);
        std::vector<MeshTopology> meshTopology;
        unsigned int vertexBufferId;
        struct VAOBinding {
            long shaderId;
            unsigned int vaoID;
        };
        std::map<unsigned int, VAOBinding> shaderToVertexArrayObject;
        unsigned int elementBufferId = 0;
        std::vector<ElementBufferData> elementBufferOffsetCount;
        int vertexCount;
        int dataSize;
        std::string name;
        std::map<std::string,Attribute> attributeByName;
        std::map<std::string,std::vector<float>> attributesFloat;
        std::map<std::string,std::vector<glm::vec2>> attributesVec2;
        std::map<std::string,std::vector<glm::vec3>> attributesVec3;
        std::map<std::string,std::vector<glm::vec4>> attributesVec4;
        std::map<std::string,std::vector<glm::i32vec4>> attributesIVec4;

        std::vector<std::vector<uint32_t>> indices;

        std::array<glm::vec3,2> boundsMinMax;

        void bind(Shader* shader);
        void bindIndexSet();

        friend class RenderPass;
        friend class Inspector;

        bool hasAttribute(std::string name);

        float lineWidth {1.0f};
        glm::vec3 location {0.0f, 0.0f, 0.0f};
        glm::vec3 rotation {0.0f, 0.0f, 0.0f};
        glm::vec3 scaling {1.0f, 1.0f, 1.0f};
        std::shared_ptr<Material> material {nullptr};
    };

    template<>
    inline const std::vector<float>& Mesh::get(std::string uniformName) {
        return attributesFloat[uniformName];
    }

    template<>
    inline const std::vector<glm::vec2>& Mesh::get(std::string uniformName) {
        return attributesVec2[uniformName];
    }

    template<>
    inline const std::vector<glm::vec3>& Mesh::get(std::string uniformName) {
        return attributesVec3[uniformName];
    }

    template<>
    inline const std::vector<glm::vec4>& Mesh::get(std::string uniformName) {
        return attributesVec4[uniformName];
    }

    template<>
    inline const std::vector<glm::i32vec4>& Mesh::get(std::string uniformName) {
        return attributesIVec4[uniformName];
    }

    // The LineContainer helper class enables fast drawing of many lines. This
    // is accomplished by gathering all lines that have the same properties and
    // storing them in a single Mesh. This minimizes the number of mesh objects
    // that are used, which in turn minimizes the number of times that shaders
    // are loaded (which is a slow operation according to Shaders.hpp).
    //
    // The performance gain is VERY significant. The timing for approximately
    // 10,000 individual segments drawn using RenderPass::drawLines(...) (which
    // is known to be slow, per the notes in RenderPass.hpp) went from
    // approximately four seconds down to less than 1/60 of a second when
    // implmented using the LineContainer class (a factor of 15K faster!)

    class LineContainer {
    public:
        // Add vertices of a specified color, line width, and topology
        void add(const std::vector<glm::vec3> & verticesIn,
                    const Color & colorIn = Color(0.0, 0.0, 0.0, 1.0f),
                    const float & lineWidthIn = 1.0f,
                    const MeshTopology & topologyIn = MeshTopology::Lines);
        // Draw the line container using renderPass
        void draw(RenderPass& renderPass);
        // Clear the container without deallocating the memory
        void clear();
        // Output the LineContainer vector sizes and capacities to std::cout
        void output();
    private:
        enum class MeshStatus {Initialized, Uninitialized};
        std::vector<Color> m_colors;
        std::vector<std::shared_ptr<Material>> m_materials;
        std::vector<MeshTopology> m_topologies;
        std::vector<float> m_lineWidths;
        std::vector<std::vector<glm::vec3>> m_vertices;
        std::vector<std::shared_ptr<sre::Mesh>> m_meshes;
        std::vector<MeshStatus> m_status;
    };

}
