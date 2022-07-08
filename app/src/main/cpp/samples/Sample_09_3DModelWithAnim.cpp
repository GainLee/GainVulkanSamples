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

#define GLM_FORCE_RADIANS

#include "Sample_09_3DModelWithAnim.h"
#include "VulkanglTFModel.h"

void Sample_09_3DModelWithAnim::set3DModelPath(std::string path)
{
    mModelPath = path;
}

void Sample_09_3DModelWithAnim::prepareSynchronizationPrimitives()
{
    // Semaphores (Used for correct command ordering)
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext                 = nullptr;

    // Semaphore used to ensures that image presentation is complete before starting to submit again
    CALL_VK(vkCreateSemaphore(
        device(), &semaphoreCreateInfo, nullptr, presentCompleteSemaphore.pHandle()));

    // Semaphore used to ensures that all commands submitted have been finished before submitting
    // the image to the queue
    CALL_VK(vkCreateSemaphore(
        device(), &semaphoreCreateInfo, nullptr, renderCompleteSemaphore.pHandle()));
}

void Sample_09_3DModelWithAnim::updateUniformBuffers()
{
    shaderData.values.projection = mCamera.matrices.perspective;
    shaderData.values.model      = mCamera.matrices.view;

    mUniformBuffer->copyFrom(&shaderData.values);
}

void Sample_09_3DModelWithAnim::setupDescriptorSetLayout()
{
    descriptorSetLayouts.ubo      = VulkanDescriptorSetLayout(device());
    descriptorSetLayouts.textures = VulkanDescriptorSetLayout(device());
    descriptorSetLayouts.node     = VulkanDescriptorSetLayout(device());

    // Descriptor set layout for passing ubo
    {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}};
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI =
            vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
        CALL_VK(vkCreateDescriptorSetLayout(
            device(), &descriptorSetLayoutCI, nullptr, descriptorSetLayouts.ubo.pHandle()));
    }

    // Descriptor set layout for passing material textures
    {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI =
            vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
        CALL_VK(vkCreateDescriptorSetLayout(
            device(), &descriptorSetLayoutCI, nullptr, descriptorSetLayouts.textures.pHandle()));
    }

    // Descriptor set layout for nodes
    {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}};
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
        CALL_VK(vkCreateDescriptorSetLayout(device(), &descriptorSetLayoutCI, nullptr, descriptorSetLayouts.node.pHandle()));
    }

    // Pipeline layout using descriptor sets (set 0 = ubo, set 1 = material, set 2 = node)
    std::vector<VkDescriptorSetLayout> setLayouts = {
        descriptorSetLayouts.ubo.handle(),
        descriptorSetLayouts.textures.handle(),
        descriptorSetLayouts.node.handle()};
    VkPipelineLayoutCreateInfo pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(
        setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));
    CALL_VK(
        vkCreatePipelineLayout(device(), &pipelineLayoutCI, nullptr, mPipelineLayout.pHandle()));
}

void Sample_09_3DModelWithAnim::setupDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes = {
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
        vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                              static_cast<uint32_t>(animModels.scene.materials.size())),
    };

    const uint32_t             maxSetCount = static_cast<uint32_t>(animModels.scene.materials.size()) + 2;
    VkDescriptorPoolCreateInfo descriptorPoolInfo =
        vks::initializers::descriptorPoolCreateInfo(poolSizes, maxSetCount);
    CALL_VK(
        vkCreateDescriptorPool(device(), &descriptorPoolInfo, nullptr, mDescriptorPool.pHandle()));
}

void Sample_09_3DModelWithAnim::prepare(JNIEnv *env)
{
    if (!mPrepared)
    {
        VulkanContextBase::prepare(env);

        prepare3DModel(env);

        initCameraView();

        prepareSynchronizationPrimitives();
        prepareUniformBuffers();
        setupDescriptorPool();
        setupDescriptorSetLayout();
        setupDescriptorSet();
        preparePipelines();

        buildCommandBuffers();

        mPrepared = true;
    }

    updateUniformBuffers();
}

void Sample_09_3DModelWithAnim::initCameraView()
{
    mCamera.type = Camera::CameraType::lookat;
    mCamera.setPosition(glm::vec3(0.0f, -0.5f, 4.0f));
}

void Sample_09_3DModelWithAnim::prepare3DModel(JNIEnv *env)
{
    vkglTF::setupAssetManager(mAssetManager);
    animModels.scene.loadFromFile(mModelPath, deviceWrapper(), mGraphicsQueue);
}

void Sample_09_3DModelWithAnim::preparePipelines()
{
    // Create the graphics pipeline used in this example
    // Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing
    // OpenGL's complex state machine A pipeline is then stored and hashed on the GPU making
    // pipeline changes very fast Note: There are still a few dynamic states that are not directly
    // part of the pipeline (but the info that they are used is)

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // The layout used for this pipeline (can be shared among multiple pipelines using the same
    // layout)
    pipelineCreateInfo.layout = mPipelineLayout.handle();
    // Renderpass this pipeline is attached to
    pipelineCreateInfo.renderPass = mRenderPass;

    // Construct the different states making up the pipeline

    // Input assembly state describes how primitives are assembled
    // This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    inputAssemblyState.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    rasterizationState.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.polygonMode                            = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode                               = VK_CULL_MODE_NONE;
    rasterizationState.frontFace                              = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthClampEnable                       = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable                = VK_FALSE;
    rasterizationState.depthBiasEnable                        = VK_FALSE;
    rasterizationState.lineWidth                              = 1.0f;

    // Color blend state describes how blend factors are calculated (if used)
    // We need one blend attachment state per color attachment (even if blending is not used)
    VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
    blendAttachmentState[0].colorWriteMask                      = 0xf;
    blendAttachmentState[0].blendEnable                         = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo colorBlendState         = {};
    colorBlendState.sType                                       = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount                             = 1;
    colorBlendState.pAttachments                                = blendAttachmentState;

    // Viewport state sets the number of viewports and scissor used in this pipeline
    // Note: This is actually overridden by the dynamic states (see below)
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount                     = 1;
    viewportState.scissorCount                      = 1;

    // Enable dynamic states
    // Most states are baked into the pipeline, but there are still a few dynamic states that can be
    // changed within a command buffer To be able to change these we need do specify which dynamic
    // states will be changed using this pipeline. Their actual states are set later on in the
    // command buffer. For this example we will set the viewport and scissor using dynamic states
    std::vector<VkDynamicState> dynamicStateEnables;
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates                   = dynamicStateEnables.data();
    dynamicState.dynamicStateCount                = static_cast<uint32_t>(dynamicStateEnables.size());

    // Depth and stencil state containing depth and stencil compare and test operations
    // We only use depth tests and want depth tests and writes to be enabled and compare with less
    // or equal
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable                       = VK_TRUE;
    depthStencilState.depthWriteEnable                      = VK_TRUE;
    depthStencilState.depthCompareOp                        = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthBoundsTestEnable                 = VK_FALSE;
    depthStencilState.back.failOp                           = VK_STENCIL_OP_KEEP;
    depthStencilState.back.passOp                           = VK_STENCIL_OP_KEEP;
    depthStencilState.back.compareOp                        = VK_COMPARE_OP_ALWAYS;
    depthStencilState.stencilTestEnable                     = VK_FALSE;
    depthStencilState.front                                 = depthStencilState.back;

    // Multi sampling state
    // This example does not make use of multi sampling (for anti-aliasing), the state must still be
    // set and passed to the pipeline
    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    multisampleState.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.pSampleMask                          = nullptr;

    // Vertex input descriptions
    // Specifies the vertex input parameters for a pipeline

    // Vertex input bindings and attributes
    VkVertexInputBindingDescription vertexInputBinding = {0, sizeof(vkglTF::Model::Vertex), VK_VERTEX_INPUT_RATE_VERTEX};

    std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Model::Vertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Model::Vertex, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vkglTF::Model::Vertex, uv0)},
        {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vkglTF::Model::Vertex, uv1)},
        {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vkglTF::Model::Vertex, joint0)},
        {5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vkglTF::Model::Vertex, weight0)}};

    // Vertex input state used for pipeline creation
    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount        = 1;
    vertexInputState.pVertexBindingDescriptions           = &vertexInputBinding;
    vertexInputState.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(vertexInputAttributes.size());
    vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

    // Shaders
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

    // Vertex shader
    shaderStages[0] = loadShader(vertFilePath,
                                 VK_SHADER_STAGE_VERTEX_BIT);
    // Fragment shader
    shaderStages[1] = loadShader(fragFilePath,
                                 VK_SHADER_STAGE_FRAGMENT_BIT);

    // Set pipeline shader stage info
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages    = shaderStages.data();

    // Assign the pipeline states to the pipeline creation info structure
    pipelineCreateInfo.pVertexInputState   = &vertexInputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pColorBlendState    = &colorBlendState;
    pipelineCreateInfo.pMultisampleState   = &multisampleState;
    pipelineCreateInfo.pViewportState      = &viewportState;
    pipelineCreateInfo.pDepthStencilState  = &depthStencilState;
    pipelineCreateInfo.renderPass          = mRenderPass;
    pipelineCreateInfo.pDynamicState       = &dynamicState;

    // Create rendering pipeline using the specified states
    CALL_VK(vkCreateGraphicsPipelines(
        device(), mPipelineCache.handle(), 1, &pipelineCreateInfo, nullptr, mPipeline.pHandle()));

    // Shader modules are no longer needed once the graphics pipeline has been created
    vkDestroyShaderModule(device(), shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device(), shaderStages[1].module, nullptr);
}

void Sample_09_3DModelWithAnim::setupDescriptorSet()
{
    // Descriptor set for scene ubo
    {
        VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(
            mDescriptorPool.handle(), descriptorSetLayouts.ubo.pHandle(), 1);
        CALL_VK(vkAllocateDescriptorSets(device(), &allocInfo, &mDescriptorSet));
        auto                 descriptor         = mUniformBuffer->getDescriptor();
        VkWriteDescriptorSet writeDescriptorSet = vks::initializers::writeDescriptorSet(mDescriptorSet,
                                                                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                                        0, &descriptor);
        vkUpdateDescriptorSets(device(), 1, &writeDescriptorSet, 0, nullptr);
    }

    // Descriptor sets for materials
    {
        for (auto &image : animModels.scene.materials)
        {
            if (image.baseColorTexture)
            {
                const VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(
                    mDescriptorPool.handle(), descriptorSetLayouts.textures.pHandle(), 1);
                CALL_VK(vkAllocateDescriptorSets(device(), &allocInfo, &image.descriptorSet));
                VkWriteDescriptorSet writeDescriptorSet =
                    vks::initializers::writeDescriptorSet(image.descriptorSet,
                                                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                          0,
                                                          &image.baseColorTexture->descriptor);
                vkUpdateDescriptorSets(device(), 1, &writeDescriptorSet, 0, nullptr);
            }
        }
    }

    // Descriptor sets for model node (matrices)
    {
        // Per-Node descriptor set
        for (auto &node : animModels.scene.nodes)
        {
            setupNodeDescriptorSet(node);
        }
    }
}

void Sample_09_3DModelWithAnim::setupNodeDescriptorSet(vkglTF::Node *node)
{
    if (node->mesh)
    {
        VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
        descriptorSetAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocInfo.descriptorPool     = mDescriptorPool.handle();
        descriptorSetAllocInfo.pSetLayouts        = descriptorSetLayouts.node.pHandle();
        descriptorSetAllocInfo.descriptorSetCount = 1;
        CALL_VK(vkAllocateDescriptorSets(device(), &descriptorSetAllocInfo, &node->mesh->uniformBuffer.descriptorSet));

        auto                 descriptor = node->mesh->uniformBuffer.buffer->getDescriptor();
        VkWriteDescriptorSet writeDescriptorSet =
            vks::initializers::writeDescriptorSet(node->mesh->uniformBuffer.descriptorSet,
                                                  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  0,
                                                  &descriptor);

        vkUpdateDescriptorSets(device(), 1, &writeDescriptorSet, 0, nullptr);
    }
    for (auto &child : node->children)
    {
        setupNodeDescriptorSet(child);
    }
}

void Sample_09_3DModelWithAnim::buildCommandBuffers()
{
    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext                    = nullptr;

    // Set clear values for all framebuffer attachments with loadOp set to clear
    // We use two attachments (color and depth) that are cleared at the start of the subpass and as
    // such we need to set clear values for both
    VkClearValue clearValues[2];
    clearValues[0].color        = {{0.0f, 0.0f, 0.2f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassBeginInfo    = {};
    renderPassBeginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext                    = nullptr;
    renderPassBeginInfo.renderPass               = mRenderPass;
    renderPassBeginInfo.renderArea.offset.x      = 0;
    renderPassBeginInfo.renderArea.offset.y      = 0;
    renderPassBeginInfo.renderArea.extent.width  = mWindow.windowWidth;
    renderPassBeginInfo.renderArea.extent.height = mWindow.windowHeight;
    renderPassBeginInfo.clearValueCount          = 2;
    renderPassBeginInfo.pClearValues             = clearValues;

    for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
    {
        // Set target frame buffer
        renderPassBeginInfo.framebuffer = frameBuffers[i];

        CALL_VK(vkBeginCommandBuffer(drawCmdBuffers[i].handle(), &cmdBufInfo));

        // Start the first sub pass specified in our default prepare pass setup by the base class
        // This will clear the color and depth attachment
        vkCmdBeginRenderPass(
            drawCmdBuffers[i].handle(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Update dynamic viewport state
        VkViewport viewport = {};
        viewport.height     = (float) mWindow.windowHeight;
        viewport.width      = (float) mWindow.windowWidth;
        viewport.minDepth   = (float) 0.0f;
        viewport.maxDepth   = (float) 1.0f;
        vkCmdSetViewport(drawCmdBuffers[i].handle(), 0, 1, &viewport);

        // Update dynamic scissor state
        VkRect2D scissor      = {};
        scissor.extent.width  = mWindow.windowWidth;
        scissor.extent.height = mWindow.windowHeight;
        scissor.offset.x      = 0;
        scissor.offset.y      = 0;
        vkCmdSetScissor(drawCmdBuffers[i].handle(), 0, 1, &scissor);

        // Bind descriptor sets describing shader binding points
        vkCmdBindDescriptorSets(drawCmdBuffers[i].handle(),
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mPipelineLayout.handle(),
                                0,
                                1,
                                &mDescriptorSet,
                                0,
                                nullptr);

        // Bind the rendering pipeline
        // The pipeline (state object) contains all states of the rendering pipeline, binding it
        // will set all the states specified at pipeline creation time
        vkCmdBindPipeline(
            drawCmdBuffers[i].handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.handle());

        const VkDeviceSize offsets[1]  = {0};
        auto               verticesBuf = animModels.scene.vertices.buffer->getBufferHandle();
        auto               indicesBuf  = animModels.scene.indices.buffer->getBufferHandle();
        vkCmdBindVertexBuffers(drawCmdBuffers[i].handle(), 0, 1, &verticesBuf, offsets);
        vkCmdBindIndexBuffer(drawCmdBuffers[i].handle(), indicesBuf, 0, VK_INDEX_TYPE_UINT32);

        for (auto node : animModels.scene.nodes)
        {
            renderNode(node, drawCmdBuffers[i].handle());
        }

        //        drawUI(drawCmdBuffers[i].handle());

        vkCmdEndRenderPass(drawCmdBuffers[i].handle());

        // Ending the prepare pass will add an implicit barrier transitioning the frame buffer color
        // attachment to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

        CALL_VK(vkEndCommandBuffer(drawCmdBuffers[i].handle()));
    }
}

void Sample_09_3DModelWithAnim::renderNode(vkglTF::Node *node, VkCommandBuffer cmd)
{
    if (node->mesh)
    {
        vkCmdBindDescriptorSets(cmd,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mPipelineLayout.handle(),
                                2,
                                1,
                                &node->mesh->uniformBuffer.descriptorSet,
                                0,
                                nullptr);

        // Render mesh primitives
        for (vkglTF::Primitive *primitive : node->mesh->primitives)
        {
            vkCmdBindDescriptorSets(cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    mPipelineLayout.handle(),
                                    1,
                                    1,
                                    &primitive->material.descriptorSet,
                                    0,
                                    nullptr);
            if (primitive->hasIndices)
            {
                vkCmdDrawIndexed(cmd, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
            }
            else
            {
                vkCmdDraw(cmd, primitive->vertexCount, 1, 0, 0);
            }
        }
    };
    for (auto child : node->children)
    {
        renderNode(child, cmd);
    }
}

void Sample_09_3DModelWithAnim::prepareUniformBuffers()
{
    // Prepare and initialize a uniform buffer block containing shader uniforms
    // Single uniforms like in OpenGL are no longer present in Vulkan. All Shader uniforms are
    // passed via uniform buffer blocksvkCreateDescriptorPool
    mUniformBuffer =
        Buffer::create(deviceWrapper(),
                       sizeof(shaderData.values),
                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    updateUniformBuffers();
}

void Sample_09_3DModelWithAnim::draw()
{
    VulkanContextBase::draw();

    if (animModels.scene.animations.size() > 0)
    {
        animationTimer += frameTimer * /*timerSpeed*/ 4;
        if (animationTimer > animModels.scene.animations[0].end)
        {
            animationTimer -= animModels.scene.animations[0].end;
        }
        animModels.scene.updateAnimation(0, animationTimer);
    }
}

void Sample_09_3DModelWithAnim::onTouchActionMove(float deltaX, float deltaY)
{
    mCamera.rotate(glm::vec3(0.0f, -deltaX * mCamera.rotationSpeed * 0.1f, 0.0f));
}

void Sample_09_3DModelWithAnim::unInit(JNIEnv *env)
{}

Sample_09_3DModelWithAnim::~Sample_09_3DModelWithAnim()
{}