// 假设这是 RetroArch Vulkan 普通模式下的渲染主循环伪代码整合
// 变量名和结构体尽量贴合 RetroArch 源码

void retroarch_vulkan_render_loop(gfx_ctx_vulkan_data_t *vk)
{
    // 假设：vk->context.num_swapchain_images 已初始化
    // 假设：vk->context.current_frame_index 已初始化为 0
    // 假设：VkSemaphore, VkFence, VkCommandBuffer 等已初始化
    // 省略窗口消息处理、输入等

    while (running) {
        // ---- 步骤1：推进 current_frame_index 并处理 fence ----
        vk->context.current_frame_index =
            (vk->context.current_frame_index + 1) % vk->context.num_swapchain_images;
        uint32_t frame_index = vk->context.current_frame_index;

        VkFence *frame_fence = &vk->context.swapchain_fences[frame_index];
        if (*frame_fence != VK_NULL_HANDLE) {
            if (vk->context.swapchain_fences_signalled[frame_index])
                vkWaitForFences(vk->context.device, 1, frame_fence, VK_TRUE, UINT64_MAX);
            vkResetFences(vk->context.device, 1, frame_fence);
        } else {
            VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            vkCreateFence(vk->context.device, &fence_info, NULL, frame_fence);
        }
        vk->context.swapchain_fences_signalled[frame_index] = false;

        // ---- 步骤2：回收上一次的 acquire semaphore ----
        if (vk->context.swapchain_wait_semaphores[frame_index] != VK_NULL_HANDLE) {
            VkSemaphore sem = vk->context.swapchain_wait_semaphores[frame_index];
            vk->context.swapchain_recycled_semaphores[vk->context.num_recycled_acquire_semaphores++] = sem;
        }
        vk->context.swapchain_wait_semaphores[frame_index] = VK_NULL_HANDLE;

        // ---- 步骤3：获取可用的 acquire semaphore ----
        VkSemaphore acquire_semaphore;
        if (vk->context.num_recycled_acquire_semaphores == 0) {
            VkSemaphoreCreateInfo sem_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            vkCreateSemaphore(vk->context.device, &sem_info, NULL,
                &vk->context.swapchain_recycled_semaphores[vk->context.num_recycled_acquire_semaphores++]);
        }
        acquire_semaphore =
            vk->context.swapchain_recycled_semaphores[--vk->context.num_recycled_acquire_semaphores];
        vk->context.swapchain_recycled_semaphores[vk->context.num_recycled_acquire_semaphores] = VK_NULL_HANDLE;

        // ---- 步骤4：调用 vkAcquireNextImageKHR 获取 swapchain image ----
        uint32_t image_index = 0;
        VkResult acquire_result = vkAcquireNextImageKHR(
            vk->context.device,
            vk->swapchain,
            UINT64_MAX,
            acquire_semaphore,
            VK_NULL_HANDLE,
            &image_index
        );
        if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
            // swapchain 失效，需重建
            rebuild_swapchain(vk);
            continue;
        } else if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR) {
            // 错误处理
            handle_acquire_error(acquire_result);
            continue;
        }

        // ---- 步骤5：记录本帧的 semaphore 以便后续 submit/present 使用 ----
        vk->context.swapchain_wait_semaphores[frame_index] = acquire_semaphore;

        // ---- 步骤6：录制并提交渲染命令 ----
        VkCommandBuffer cmd = vk->context.command_buffers[frame_index];
        record_render_commands(cmd, image_index); // 伪函数

        // 提交命令到队列，等待 acquire_semaphore，渲染完成 signal frame_fence
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &acquire_semaphore,
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &vk->context.swapchain_semaphores[frame_index]
        };

        vkQueueSubmit(
            vk->context.queue,
            1,
            &submit_info,
            *frame_fence
        );
        vk->context.swapchain_fences_signalled[frame_index] = true;

        // ---- 步骤7：Present 图像 ----
        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &vk->context.swapchain_semaphores[frame_index],
            .swapchainCount = 1,
            .pSwapchains = &vk->swapchain,
            .pImageIndices = &image_index
        };
        vkQueuePresentKHR(vk->context.queue, &present_info);

        // 省略：窗口消息处理、退出检测等
    }
}