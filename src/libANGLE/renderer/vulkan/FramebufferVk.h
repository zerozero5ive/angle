//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FramebufferVk.h:
//    Defines the class interface for FramebufferVk, implementing FramebufferImpl.
//

#ifndef LIBANGLE_RENDERER_VULKAN_FRAMEBUFFERVK_H_
#define LIBANGLE_RENDERER_VULKAN_FRAMEBUFFERVK_H_

#include "libANGLE/renderer/FramebufferImpl.h"
#include "libANGLE/renderer/RenderTargetCache.h"
#include "libANGLE/renderer/vulkan/BufferVk.h"
#include "libANGLE/renderer/vulkan/CommandGraph.h"
#include "libANGLE/renderer/vulkan/vk_cache_utils.h"

namespace rx
{
class RendererVk;
class RenderTargetVk;
class WindowSurfaceVk;

class FramebufferVk : public FramebufferImpl, public vk::CommandGraphResource
{
  public:
    // Factory methods so we don't have to use constructors with overloads.
    static FramebufferVk *CreateUserFBO(const gl::FramebufferState &state);

    // The passed-in SurfaceVK must be destroyed after this FBO is destroyed. Our Surface code is
    // ref-counted on the number of 'current' contexts, so we shouldn't get any dangling surface
    // references. See Surface::setIsCurrent(bool).
    static FramebufferVk *CreateDefaultFBO(const gl::FramebufferState &state,
                                           WindowSurfaceVk *backbuffer);

    ~FramebufferVk() override;
    void destroy(const gl::Context *context) override;

    gl::Error discard(const gl::Context *context, size_t count, const GLenum *attachments) override;
    gl::Error invalidate(const gl::Context *context,
                         size_t count,
                         const GLenum *attachments) override;
    gl::Error invalidateSub(const gl::Context *context,
                            size_t count,
                            const GLenum *attachments,
                            const gl::Rectangle &area) override;

    gl::Error clear(const gl::Context *context, GLbitfield mask) override;
    gl::Error clearBufferfv(const gl::Context *context,
                            GLenum buffer,
                            GLint drawbuffer,
                            const GLfloat *values) override;
    gl::Error clearBufferuiv(const gl::Context *context,
                             GLenum buffer,
                             GLint drawbuffer,
                             const GLuint *values) override;
    gl::Error clearBufferiv(const gl::Context *context,
                            GLenum buffer,
                            GLint drawbuffer,
                            const GLint *values) override;
    gl::Error clearBufferfi(const gl::Context *context,
                            GLenum buffer,
                            GLint drawbuffer,
                            GLfloat depth,
                            GLint stencil) override;

    GLenum getImplementationColorReadFormat(const gl::Context *context) const override;
    GLenum getImplementationColorReadType(const gl::Context *context) const override;
    gl::Error readPixels(const gl::Context *context,
                         const gl::Rectangle &area,
                         GLenum format,
                         GLenum type,
                         void *pixels) override;

    gl::Error blit(const gl::Context *context,
                   const gl::Rectangle &sourceArea,
                   const gl::Rectangle &destArea,
                   GLbitfield mask,
                   GLenum filter) override;

    bool checkStatus(const gl::Context *context) const override;

    gl::Error syncState(const gl::Context *context,
                        const gl::Framebuffer::DirtyBits &dirtyBits) override;

    gl::Error getSamplePosition(const gl::Context *context,
                                size_t index,
                                GLfloat *xy) const override;
    RenderTargetVk *getDepthStencilRenderTarget() const;
    const vk::RenderPassDesc &getRenderPassDesc();
    vk::Error getCommandBufferForDraw(ContextVk *contextVk,
                                      vk::CommandBuffer **commandBufferOut,
                                      vk::RecordingMode *modeOut);

    // Internal helper function for readPixels operations.
    vk::Error readPixelsImpl(const gl::Context *context,
                             const gl::Rectangle &area,
                             const PackPixelsParams &packPixelsParams,
                             const VkImageAspectFlags &copyAspectFlags,
                             RenderTargetVk *renderTarget,
                             void *pixels);

    const gl::Extents &getReadImageExtents() const;

    gl::DrawBufferMask getEmulatedAlphaAttachmentMask();
    RenderTargetVk *getColorReadRenderTarget() const;

  private:
    FramebufferVk(const gl::FramebufferState &state);
    FramebufferVk(const gl::FramebufferState &state, WindowSurfaceVk *backbuffer);

    gl::Error blitUsingCopy(vk::CommandBuffer *commandBuffer,
                            const gl::Rectangle &readArea,
                            const gl::Rectangle &destArea,
                            RenderTargetVk *readRenderTarget,
                            RenderTargetVk *drawRenderTarget,
                            const gl::Rectangle *scissor,
                            bool blitDepthBuffer,
                            bool blitStencilBuffer);
    gl::Error blitWithReadback(const gl::Context *context,
                               const gl::Rectangle &sourceArea,
                               const gl::Rectangle &destArea,
                               bool blitDepthBuffer,
                               bool blitStencilBuffer,
                               vk::CommandBuffer *commandBuffer,
                               RenderTargetVk *readRenderTarget,
                               RenderTargetVk *drawRenderTarget);

    vk::Error getFramebuffer(RendererVk *rendererVk, vk::Framebuffer **framebufferOut);

    gl::Error clearWithClearAttachments(ContextVk *contextVk,
                                        bool clearColor,
                                        bool clearDepth,
                                        bool clearStencil);
    gl::Error clearWithDraw(const gl::Context *context, VkColorComponentFlags colorMaskFlags);
    void updateActiveColorMasks(size_t colorIndex, bool r, bool g, bool b, bool a);

    gl::Error blitImpl(ContextVk *contextVk,
                       vk::CommandBuffer *commandBuffer,
                       const gl::Rectangle &readRectIn,
                       const gl::Rectangle &drawRectIn,
                       RenderTargetVk *readRenderTarget,
                       RenderTargetVk *drawRenderTarget,
                       GLenum filter,
                       const gl::Rectangle *scissor,
                       bool colorBlit,
                       bool depthBlit,
                       bool stencilBlit,
                       bool flipSource,
                       bool flipDest);

    WindowSurfaceVk *mBackbuffer;

    Optional<vk::RenderPassDesc> mRenderPassDesc;
    vk::Framebuffer mFramebuffer;
    RenderTargetCache<RenderTargetVk> mRenderTargetCache;

    // These two variables are used to quickly compute if we need to do a masked clear. If a color
    // channel is masked out, we check against the Framebuffer Attachments (RenderTargets) to see
    // if the masked out channel is present in any of the attachments.
    VkColorComponentFlags mActiveColorComponents;
    gl::DrawBufferMask mActiveColorComponentMasksForClear[4];
    vk::DynamicBuffer mReadPixelsBuffer;
    vk::DynamicBuffer mBlitPixelBuffer;

    // When we draw to the framebuffer, and the real format has an alpha channel but the format of
    // the framebuffer does not, we need to mask out the alpha channel. This DrawBufferMask will
    // contain the mask to apply to the alpha channel when drawing.
    gl::DrawBufferMask mEmulatedAlphaAttachmentMask;
};
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_FRAMEBUFFERVK_H_
