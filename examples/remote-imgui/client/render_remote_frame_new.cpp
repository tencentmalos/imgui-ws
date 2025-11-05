void SimpleOpenGLClient::RenderRemoteFrameOnly() {
    if (!deserializer_.HasValidFrame()) { return; }

    const spatial::debugger::FrameData* frame_data = deserializer_.GetFrameData();
    if (!frame_data || frame_data->header.cmd_lists_count == 0) { return; }

    // Create a standalone ImDrawData structure from cmdlist-based remote frame data
    ImDrawData remote_draw_data;
    remote_draw_data.Valid = true;
    remote_draw_data.CmdListsCount = frame_data->header.cmd_lists_count;
    remote_draw_data.CmdLists = nullptr;
    remote_draw_data.TotalIdxCount = 0;
    remote_draw_data.TotalVtxCount = 0;
    remote_draw_data.DisplayPos =
            ImVec2(frame_data->header.display_pos[0], frame_data->header.display_pos[1]);
    remote_draw_data.DisplaySize =
            ImVec2(frame_data->header.display_size[0], frame_data->header.display_size[1]);
    remote_draw_data.FramebufferScale = ImVec2(frame_data->header.framebuffer_scale[0],
                                               frame_data->header.framebuffer_scale[1]);

    // Update ImGui IO with server display settings
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = remote_draw_data.DisplaySize;
    io.DisplayFramebufferScale = remote_draw_data.FramebufferScale;

    // Allocate draw lists array
    remote_draw_data.CmdLists = new ImDrawList*[frame_data->header.cmd_lists_count];

    // Create draw lists from cmdlist-based remote data
    for (uint32_t i = 0; i < frame_data->header.cmd_lists_count; i++) {
        if (i >= frame_data->cmd_lists.size()) {
            remote_draw_data.CmdLists[i] = nullptr;
            continue;
        }

        const spatial::debugger::CmdListData& src_cmdlist = frame_data->cmd_lists[i];

        if (src_cmdlist.vertex_buffer.empty() || src_cmdlist.index_buffer.empty()) {
            remote_draw_data.CmdLists[i] = nullptr;
            continue;
        }

        // Create new draw list
        ImDrawList* new_list = new ImDrawList(ImGui::GetDrawListSharedData());

        // Copy vertex buffer directly using memcpy for efficiency
        new_list->VtxBuffer.resize(src_cmdlist.vertex_buffer.size());
        memcpy(new_list->VtxBuffer.Data, src_cmdlist.vertex_buffer.data(),
               src_cmdlist.vertex_buffer.size() * sizeof(ImDrawVert));

        // Copy index buffer directly using memcpy for efficiency
        new_list->IdxBuffer.resize(src_cmdlist.index_buffer.size());
        memcpy(new_list->IdxBuffer.Data, src_cmdlist.index_buffer.data(),
               src_cmdlist.index_buffer.size() * sizeof(ImDrawIdx));

        // Copy draw commands
        new_list->CmdBuffer.resize(src_cmdlist.draw_commands.size());
        for (size_t j = 0; j < src_cmdlist.draw_commands.size(); j++) {
            const spatial::debugger::DrawCmd& src_cmd = src_cmdlist.draw_commands[j];
            ImDrawCmd& dst_cmd = new_list->CmdBuffer[j];

            dst_cmd.ElemCount = src_cmd.idx_count;
            dst_cmd.ClipRect.x = src_cmd.clip_rect[0] / 1000.0f;
            dst_cmd.ClipRect.y = src_cmd.clip_rect[1] / 1000.0f;
            dst_cmd.ClipRect.z = src_cmd.clip_rect[2] / 1000.0f;
            dst_cmd.ClipRect.w = src_cmd.clip_rect[3] / 1000.0f;

            // Map texture IDs: if it's the default font texture (ID 1), use our client font texture
            if (src_cmd.texture_id == 1) {
                dst_cmd.TextureId =
                        reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(font_texture_id_));
                // Debug: Only log the first font texture mapping to avoid spam
                static bool font_texture_logged = false;
                if (!font_texture_logged) {
                    std::cout << "Mapped server font texture ID " << src_cmd.texture_id
                              << " to client texture ID " << font_texture_id_ << std::endl;
                    font_texture_logged = true;
                }
            } else {
                dst_cmd.TextureId = (ImTextureID) (uintptr_t) src_cmd.texture_id;
            }

            // Use the transmitted offsets directly - these are relative to this cmdlist's buffers
            dst_cmd.IdxOffset = src_cmd.idx_offset;
            dst_cmd.VtxOffset = src_cmd.vtx_offset;

            // Handle UserCallback restoration
            if (src_cmd.user_callback != 0) {
                // For remote ImGui, we need to provide appropriate callbacks
                // Since we can't serialize function pointers, we'll provide standard callbacks
                if (src_cmd.user_callback == 1) {
                    // This was a UserCallback - provide a default rendering callback
                    dst_cmd.UserCallback = ImDrawCallback_ResetRenderState;

                    // Allocate and copy user callback data
                    if (src_cmd.user_callback_data_size > 0 &&
                        !src_cmd.user_callback_data.empty()) {
                        // Allocate memory for callback data (will be freed by ImGui)
                        void* callback_data = malloc(src_cmd.user_callback_data_size);
                        if (callback_data) {
                            memcpy(callback_data, src_cmd.user_callback_data.data(),
                                   src_cmd.user_callback_data_size);
                            dst_cmd.UserCallbackData = callback_data;
                        } else {
                            dst_cmd.UserCallbackData = nullptr;
                        }
                    } else {
                        dst_cmd.UserCallbackData = nullptr;
                    }
                } else {
                    dst_cmd.UserCallback = nullptr;
                    dst_cmd.UserCallbackData = nullptr;
                }
            } else {
                dst_cmd.UserCallback = nullptr;
                dst_cmd.UserCallbackData = nullptr;
            }
        }

        remote_draw_data.CmdLists[i] = new_list;
        remote_draw_data.TotalVtxCount += src_cmdlist.vertex_buffer.size();
        remote_draw_data.TotalIdxCount += src_cmdlist.index_buffer.size();
    }

    // Debug: Print cmdlist-based rendering information
    std::cout << "Rendering cmdlist-based remote frame data with "
              << remote_draw_data.CmdListsCount << " draw lists, "
              << remote_draw_data.TotalVtxCount << " vertices, "
              << remote_draw_data.TotalIdxCount << " indices" << std::endl;

    if (font_texture_id_ != 0) {
        std::cout << "Font texture ID available: " << font_texture_id_ << std::endl;

        // Ensure font texture is bound properly
        glBindTexture(GL_TEXTURE_2D, font_texture_id_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        std::cout << "Warning: No font texture available!" << std::endl;
    }

    // Render the remote frame data directly
    ImGui_ImplOpenGL3_RenderDrawData(&remote_draw_data);

    // Clean up allocated draw lists and user callback data
    for (int i = 0; i < remote_draw_data.CmdListsCount; i++) {
        if (remote_draw_data.CmdLists[i]) {
            // Free UserCallbackData memory
            for (int cmd_idx = 0; cmd_idx < remote_draw_data.CmdLists[i]->CmdBuffer.Size;
                 cmd_idx++) {
                ImDrawCmd& cmd = remote_draw_data.CmdLists[i]->CmdBuffer[cmd_idx];
                if (cmd.UserCallbackData) {
                    free(cmd.UserCallbackData);
                    cmd.UserCallbackData = nullptr;
                }
            }
            delete remote_draw_data.CmdLists[i];
        }
    }
    delete[] remote_draw_data.CmdLists;
}