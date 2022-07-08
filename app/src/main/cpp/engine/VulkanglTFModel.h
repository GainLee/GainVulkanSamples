/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 by Gain
 * Copyright (C) 2022 by Sascha Willems - www.saschawillems.de
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <fstream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "VulkanDeviceWrapper.hpp"
#include "VulkanResources.h"

/*#include <ktx/include/ktx.h>
#include <ktx/include/ktxvulkan.h>*/

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#    define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif

#include "../util/tinygltf/tiny_gltf.h"

#if defined(__ANDROID__)

#    include <android/asset_manager.h>

#endif

// Changing this value here also requires changing it in the vertex shader
#define MAX_NUM_JOINTS 128u

namespace vkglTF
{
#ifdef __ANDROID__
#    ifdef TINYGLTF_ANDROID_LOAD_FROM_ASSETS
void setupAssetManager(AAssetManager *assetManager);
#    endif
#endif

struct Node;

struct BoundingBox
{
    glm::vec3 min;
    glm::vec3 max;
    bool      valid = false;
    BoundingBox();
    BoundingBox(glm::vec3 min, glm::vec3 max);
    BoundingBox getAABB(glm::mat4 m);
};

struct TextureSampler
{
    VkFilter             magFilter;
    VkFilter             minFilter;
    VkSamplerAddressMode addressModeU;
    VkSamplerAddressMode addressModeV;
    VkSamplerAddressMode addressModeW;
};

struct Texture
{
    std::shared_ptr<vks::VulkanDeviceWrapper> device;
    VkImage                                   image;
    VkImageLayout                             imageLayout;
    VkDeviceMemory                            deviceMemory;
    VkImageView                               view;
    uint32_t                                  width, height;
    uint32_t                                  mipLevels;
    uint32_t                                  layerCount;
    VkDescriptorImageInfo                     descriptor;
    VkSampler                                 sampler;

    void updateDescriptor();

    void destroy();

    void fromglTfImage(tinygltf::Image &gltfimage, TextureSampler textureSampler,
                       std::shared_ptr<vks::VulkanDeviceWrapper> device, VkQueue copyQueue);
};

struct Material
{
    enum AlphaMode
    {
        ALPHAMODE_OPAQUE,
        ALPHAMODE_MASK,
        ALPHAMODE_BLEND
    };
    AlphaMode        alphaMode       = ALPHAMODE_OPAQUE;
    float            alphaCutoff     = 1.0f;
    float            metallicFactor  = 1.0f;
    float            roughnessFactor = 1.0f;
    glm::vec4        baseColorFactor = glm::vec4(1.0f);
    glm::vec4        emissiveFactor  = glm::vec4(1.0f);
    vkglTF::Texture *baseColorTexture;
    vkglTF::Texture *metallicRoughnessTexture;
    vkglTF::Texture *normalTexture;
    vkglTF::Texture *occlusionTexture;
    vkglTF::Texture *emissiveTexture;

    struct TexCoordSets
    {
        uint8_t baseColor          = 0;
        uint8_t metallicRoughness  = 0;
        uint8_t specularGlossiness = 0;
        uint8_t normal             = 0;
        uint8_t occlusion          = 0;
        uint8_t emissive           = 0;
    } texCoordSets;
    struct Extension
    {
        vkglTF::Texture *specularGlossinessTexture;
        vkglTF::Texture *diffuseTexture;
        glm::vec4        diffuseFactor  = glm::vec4(1.0f);
        glm::vec3        specularFactor = glm::vec3(0.0f);
    } extension;
    struct PbrWorkflows
    {
        bool metallicRoughness  = true;
        bool specularGlossiness = false;
    } pbrWorkflows;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

struct Primitive
{
    uint32_t  firstIndex;
    uint32_t  indexCount;
    uint32_t  vertexCount;
    Material &material;

    bool        hasIndices;
    BoundingBox bb;

    void setBoundingBox(glm::vec3 min, glm::vec3 max);

    Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material &material);
};

struct Mesh
{
    std::shared_ptr<vks::VulkanDeviceWrapper> device;

    std::vector<Primitive *> primitives;
    BoundingBox              bb;
    BoundingBox              aabb;

    struct UniformBuffer
    {
        std::unique_ptr<gain::Buffer> buffer;
        VkDescriptorSet               descriptorSet = VK_NULL_HANDLE;
    } uniformBuffer;

    struct UniformBlock
    {
        glm::mat4 matrix;
        glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
        float     jointcount{0};
    } uniformBlock;

    Mesh(std::shared_ptr<vks::VulkanDeviceWrapper> device, glm::mat4 matrix);

    ~Mesh();

    void setBoundingBox(glm::vec3 min, glm::vec3 max);
};

struct Skin
{
    std::string            name;
    Node *                 skeletonRoot = nullptr;
    std::vector<glm::mat4> inverseBindMatrices;
    std::vector<Node *>    joints;
};

struct Node
{
    Node *              parent;
    uint32_t            index;
    std::vector<Node *> children;
    glm::mat4           matrix;
    std::string         name;
    Mesh *              mesh;
    Skin *              skin;
    int32_t             skinIndex = -1;
    glm::vec3           translation{};
    glm::vec3           scale{1.0f};
    glm::quat           rotation{};
    BoundingBox         bvh;
    BoundingBox         aabb;

    glm::mat4 localMatrix();

    glm::mat4 getMatrix();

    void update();

    ~Node();
};

struct AnimationChannel
{
    enum PathType
    {
        TRANSLATION,
        ROTATION,
        SCALE
    };
    PathType path;
    Node *   node;
    uint32_t samplerIndex;
};

struct AnimationSampler
{
    enum InterpolationType
    {
        LINEAR,
        STEP,
        CUBICSPLINE
    };
    InterpolationType      interpolation;
    std::vector<float>     inputs;
    std::vector<glm::vec4> outputsVec4;
};

struct Animation
{
    std::string                   name;
    std::vector<AnimationSampler> samplers;
    std::vector<AnimationChannel> channels;
    float                         start = std::numeric_limits<float>::max();
    float                         end   = std::numeric_limits<float>::min();
};

struct Model
{
    std::shared_ptr<vks::VulkanDeviceWrapper> device;

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv0;
        glm::vec2 uv1;
        glm::vec4 joint0;
        glm::vec4 weight0;
    };

    struct Vertices
    {
        std::unique_ptr<gain::Buffer> buffer;
    } vertices;
    struct Indices
    {
        int                           count;
        std::unique_ptr<gain::Buffer> buffer;
    } indices;

    glm::mat4 aabb;

    std::vector<Node *> nodes;
    std::vector<Node *> linearNodes;

    std::vector<Skin *> skins;

    std::vector<Texture>        textures;
    std::vector<TextureSampler> textureSamplers;
    std::vector<Material>       materials;
    std::vector<Animation>      animations;
    std::vector<std::string>    extensions;

    struct Dimensions
    {
        glm::vec3 min = glm::vec3(FLT_MAX);
        glm::vec3 max = glm::vec3(-FLT_MAX);
    } dimensions;

    void                 destroy(VkDevice device);
    void                 loadNode(vkglTF::Node *parent, const tinygltf::Node &node, uint32_t nodeIndex, const tinygltf::Model &model, std::vector<uint32_t> &indexBuffer, std::vector<Vertex> &vertexBuffer, float globalscale);
    void                 loadSkins(tinygltf::Model &gltfModel);
    void                 loadTextures(tinygltf::Model &gltfModel, std::shared_ptr<vks::VulkanDeviceWrapper> device, VkQueue transferQueue);
    VkSamplerAddressMode getVkWrapMode(int32_t wrapMode);
    VkFilter             getVkFilterMode(int32_t filterMode);
    void                 loadTextureSamplers(tinygltf::Model &gltfModel);
    void                 loadMaterials(tinygltf::Model &gltfModel);
    void                 loadAnimations(tinygltf::Model &gltfModel);
    void                 loadFromFile(std::string filename, std::shared_ptr<vks::VulkanDeviceWrapper> device, VkQueue transferQueue, float scale = 1.0f);
    void                 drawNode(Node *node, VkCommandBuffer commandBuffer);
    void                 draw(VkCommandBuffer commandBuffer);
    void                 calculateBoundingBox(Node *node, Node *parent);
    void                 getSceneDimensions();
    void                 updateAnimation(uint32_t index, float time);
    Node *               findNode(Node *parent, uint32_t index);
    Node *               nodeFromIndex(uint32_t index);
};
}        // namespace vkglTF