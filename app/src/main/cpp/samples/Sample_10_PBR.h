/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Gain
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

#ifndef GAINVULKANSAMPLE_SAMPLE_10_PBR_H
#define GAINVULKANSAMPLE_SAMPLE_10_PBR_H

#include <VulkanContextBase.h>
#include <VulkanImageWrapper.h>
#include <VulkanglTFModel.h>
#include <array>

#include "VulkanInitializers.hpp"

class Sample_10_PBR : public VulkanContextBase
{
  private:
    void initCameraView();

    void prepareSynchronizationPrimitives();

    void updateUniformBuffers();

    void setupDescriptorSetLayout();

    void setupDescriptorPool();

    void prepare3DModel(JNIEnv *env);

    void setupNodeDescriptorSet(vkglTF::Node *node);

    void renderNode(vkglTF::Node *node, uint32_t cbIndex, vkglTF::Material::AlphaMode alphaMode);

    void generateCubemaps();

    void generateBRDFLUT();

    std::string mModelPath;

    struct Models
    {
        vkglTF::Model scene;
        vkglTF::Model skybox;
    } pbrModels;

    struct Textures
    {
        std::unique_ptr<Image> environmentCube;
        std::unique_ptr<Image> empty;
        std::unique_ptr<Image> lutBrdf;
        std::unique_ptr<Image> irradianceCube;
        std::unique_ptr<Image> prefilteredCube;
    } textures;

    struct UBOMatrices
    {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
        glm::vec3 camPos;
    } shaderValuesScene, shaderValuesSkybox;

    struct shaderValuesParams
    {
        glm::vec4 lightDir;
        float     exposure                 = 4.5f;
        float     gamma                    = 2.2f;
        float     prefilteredCubeMipLevels = 2;
        float     scaleIBLAmbient          = 1.0f;
        float     debugViewInputs          = 0;
        float     debugViewEquation        = 0;
    } shaderValuesParams;

    struct UniformBufferSet
    {
        std::unique_ptr<vks::Buffer> scene;
        std::unique_ptr<vks::Buffer> skybox;
        std::unique_ptr<vks::Buffer> params;
    };

    struct Pipelines
    {
        VulkanPipeline skybox        = VulkanPipeline(VK_NULL_HANDLE);
        VulkanPipeline pbr           = VulkanPipeline(VK_NULL_HANDLE);
        VulkanPipeline pbrAlphaBlend = VulkanPipeline(VK_NULL_HANDLE);
    } pipelines;

    struct DescriptorSetLayouts
    {
        VulkanDescriptorSetLayout scene    = VulkanDescriptorSetLayout(VK_NULL_HANDLE);
        VulkanDescriptorSetLayout material = VulkanDescriptorSetLayout(VK_NULL_HANDLE);
        VulkanDescriptorSetLayout node     = VulkanDescriptorSetLayout(VK_NULL_HANDLE);
    } descriptorSetLayouts;

    struct DescriptorSets
    {
        VkDescriptorSet scene;
        VkDescriptorSet skybox;
    };
    std::vector<DescriptorSets> descriptorSets;

    std::vector<UniformBufferSet> uniformBuffers;

    struct PushConstBlockMaterial
    {
        glm::vec4 baseColorFactor;
        glm::vec4 emissiveFactor;
        glm::vec4 diffuseFactor;
        glm::vec4 specularFactor;
        float     workflow;
        int       colorTextureSet;
        int       PhysicalDescriptorTextureSet;
        int       normalTextureSet;
        int       occlusionTextureSet;
        int       emissiveTextureSet;
        float     metallicFactor;
        float     roughnessFactor;
        float     alphaMask;
        float     alphaMaskCutoff;
    } pushConstBlockMaterial;

    enum PBRWorkflows
    {
        PBR_WORKFLOW_METALLIC_ROUGHNESS = 0,
        PBR_WORKFLOW_SPECULAR_GLOSINESS = 1
    };

    bool displayBackground = true;

  public:
    Sample_10_PBR() :
        VulkanContextBase("shaders/shader_10_3dmodel_pbr.vert.spv",
                          "shaders/shader_10_3dmodel_pbr.frag.spv")
    {
        settings.overlay = false;
    }

    void set3DModelPath(std::string path);

    virtual void prepare(JNIEnv *env) override;

    virtual void preparePipelines() override;

    virtual void setupDescriptorSet();

    virtual void buildCommandBuffers() override;

    virtual void prepareUniformBuffers();

    virtual void draw();

    virtual void onTouchActionMove(float deltaX, float deltaY);

    virtual void unInit(JNIEnv *env) override;

    ~Sample_10_PBR();
};

#endif        // GAINVULKANSAMPLE_SAMPLE_10_PBR_H
