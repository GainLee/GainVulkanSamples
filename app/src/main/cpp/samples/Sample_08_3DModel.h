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

#ifndef GAINVULKANSAMPLE_SAMPLE_08_3DMODEL_H
#define GAINVULKANSAMPLE_SAMPLE_08_3DMODEL_H

#include <VulkanContextBase.h>
#include <VulkanImageWrapper.h>
#include <array>

#include "VulkanInitializers.hpp"
#include "VulkanglTFModel.h"

class Sample_08_3DModel : public VulkanContextBase
{
  private:
    void initCameraView();

    void prepareSynchronizationPrimitives();

    void updateUniformBuffers();

    void setupDescriptorSetLayout();

    void setupDescriptorPool();

    void prepare3DModel(JNIEnv *env);

    void setupNodeDescriptorSet(vkglTF::Node *node);

    void renderNode(vkglTF::Node *node, VkCommandBuffer cmd);

    std::string mModelPath;

    struct Models
    {
        vkglTF::Model scene;
    } models;

    struct DescriptorSetLayouts
    {
        VulkanDescriptorSetLayout ubo      = VulkanDescriptorSetLayout(VK_NULL_HANDLE);
        VulkanDescriptorSetLayout textures = VulkanDescriptorSetLayout(VK_NULL_HANDLE);
        VulkanDescriptorSetLayout node     = VulkanDescriptorSetLayout(VK_NULL_HANDLE);
    } descriptorSetLayouts;

    struct ShaderData
    {
        struct Values
        {
            glm::mat4 projection;
            glm::mat4 model;
            glm::vec4 lightPos = glm::vec4(0.0f, 5.0f, -5.0f, 1.0f);
        } values;
    } shaderData;

  public:
    Sample_08_3DModel() :
        VulkanContextBase("shaders/shader_08_3dmodel.vert.spv",
                          "shaders/shader_08_3dmodel.frag.spv")
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

    ~Sample_08_3DModel();
};

#endif        // GAINVULKANSAMPLE_SAMPLE_08_3DMODEL_H
