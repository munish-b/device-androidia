/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2014
 * Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and confidential
 * information of Intel or its suppliers and licensors. The Material is protected by
 * worldwide copyright and trade secret laws and treaty provisions. No part of the
 * Material may be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intels prior express
 * written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel
 * or otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 *
 */

#ifndef __HwcTestReferenceComposer_h__
#define __HwcTestReferenceComposer_h__

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#ifndef EGL_EGLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <utils/Errors.h>
#include <utils/StrongPointer.h>
#include <ui/GraphicBuffer.h>

#include "Hwcval.h"
#include <hardware/hwcomposer_defs.h>

#include "DrmShimBuffer.h"
#include "HwcTestDefs.h"

class HwcContext;

class HwcTestReferenceComposer
{
public:

    HwcTestReferenceComposer();
    virtual ~HwcTestReferenceComposer();

    virtual android::status_t Compose(uint32_t numSources,
                                      hwcval_layer_t *source,
                                      hwcval_layer_t *target,
                                      bool waitForFences);

    // Make a duplicate of a gralloc buffer
    android::sp<android::GraphicBuffer> CopyBuf(buffer_handle_t handle);

    bool            mErrorOccurred; // Set if any GL error occurred during the composition

private:

    typedef enum
    {
        BACKGROUND_NONE,
        BACKGROUND_CLEAR,
        BACKGROUND_LOAD
    } EBackground;

    static bool getGLError(const char* operation);
    bool getEGLError(const char* operation);
    bool lazyCreate();
    inline bool isCreated() const;
    void destroy();

    android::status_t beginFrame(uint32_t numSources,
                                 const hwcval_layer_t *source,
                                 const hwcval_layer_t *target);
    android::status_t draw(const hwcval_layer_t *layer, uint32_t index);
    android::status_t endFrame();

    bool isFormatSupportedAsOutput(int32_t format);

    bool attachToFBO(GLuint textureId);

    void setTexture(const hwcval_layer_t *layer, uint32_t texturingUnit,
                    bool *pEGLImageCreated, bool *pTextureCreated,
                    bool *pTextureSet,
                    android::sp<android::GraphicBuffer> *pGraphicBuffer,
                    EGLImageKHR *pEGLImage, GLuint *pTextureId, int filter);

    android::status_t bindTexture(GLuint texturingUnit, GLuint textureId);

    Hwcval::Mutex mComposeMutex;


    /** \brief Helper class to save and restore the current GL context
     */
    class GLContextSaver
    {
    public:
        GLContextSaver(HwcTestReferenceComposer* refCmp);
        ~GLContextSaver();
    private:
        HwcTestReferenceComposer* mRefCmp;
        bool m_saved;
        EGLSurface mPrevDisplay;
        EGLSurface mPrevDrawSurface;
        EGLSurface mPrevReadSurface;
        EGLContext mPrevContext;
    };

    /** \brief OpenGL shader

    Provides the common operations for OpenGL shaders.
    */

    class CShader
    {
    public:
        CShader();
        ~CShader();

        bool lazyCreate(GLenum shaderType, const char* source);
        bool isCreated() const;
        void destroy();

        GLuint getId() const;

    private:
        bool m_isIdValid;
        GLuint m_id;
    };

    /** \brief OpenGL program

    Provides the common operations for OpenGL programs: linking shaders (a shader
    for each pipeline stage), binding the program (use) and attribute location
    querying.
    */

    class CProgram
    {
    public:
        CProgram();
        ~CProgram();

        bool lazyCreate(unsigned int numShaders, ...);
        bool isCreated() const;
        void destroy();

        GLuint getId() const;
        bool use();

    private:
        bool m_isIdValid;
        GLuint m_id;
    };

    /**
     Provides any GL program needed for compositing a rectangular region
     that a set of layers is known to fully cover. Each of the programs
     is composed by a vertex shader and a fragment shader.

     The choice of a program for a given region depends on two
     parameters:

     - The number of layers, being valid the values between 0 and
       maxNumLayers (including 0 and maxNumLayers).

     - The opaqueness of the first layer (if any layers are provided).

     - The background contents, that can be either black pixels or the
       current framebuffer contents.

       Only in case there are 0 layers or the first layer is not opaque
       a choice has to be made regarding to the background contents.
       Otherwise the choice of the background contents is ignored as the
       background is occluded and has no effect.

     The shaders are lazily generated and compiled upon invokation of
     any of the getShader method overloads, being the supported use
     cases:

     1) Providing no layers and only wanting to clear the framebuffer by
        means of

        bool bindClear();

     2) Providing one or more layers, being the first one opaque and the
        others (if any others) non-opaque, using

        bool bindBackgroundNone(uint32_t numLayers);

     3) Providing one or more layers, none of the opaque and expect a black
        background to be implicitly present behind the first layer, using

        bool bindBackgroundConstant(uint32_t numLayers);

     4) Providing one or more layers, none of the opaque and expect the
        blending to happen on top of the previous contents of the frame
        buffer, using

        bool bindBackgroundLoad(uint32_t numLayers, uint32_t framebufferWidth, uint32_t framebufferHeight);

     It is an invalid use cases having an opaque layer other than the
     first, because that proves the AlignedHwcTestReferenceComposer user failed to
     remove some occluded layers.
     */

    class CProgramStore
    {
    public:
        enum { maxNumLayers = 1 };

        CProgramStore();
        ~CProgramStore();

        void destroy();

        bool bind(uint32_t planeAlpha, bool destIsNV12, bool opaque, bool preMult);

        GLint getPositionVertexIn() const;
        GLint getTexCoordVertexIn() const;

        class CRendererProgram : public CProgram
        {
        // Single layer support only
        public:
            CRendererProgram();
            virtual ~CRendererProgram();
            bool getLocations();

            GLint getPositionVertexIn() const { return m_vinPosition; }
            GLint getTexCoordVertexIn() const { return m_vinTexCoord; }

            bool setPlaneAlphaUniform(float planeAlphas);

        protected:
            GLint m_vinPosition;
            GLint m_vinTexCoord;
            GLint m_uPlaneAlpha;
            GLfloat m_planeAlpha;
        };

        static bool lazyCreateProgram(
            CRendererProgram *program,
            uint32_t numLayers,
            uint32_t opaqueLayerMask,
            uint32_t premultLayerMask,
            bool renderToNV12);

    private:

        static bool getLocations(
            GLint programId,
            GLint *pvinPosition,
            GLint *pvinTexCoord,
            GLint* puPlaneAlpha,
            GLfloat* pPlaneAlpha);

        // Indices (each has value 1 for true).
        // 0 - dest is NV12
        // 1 - blending format premult i.e. has per-pixel alpha.
        // 2 - source is opaque (i.e. does not have valid plane alpha)
        CRendererProgram mPrograms[2][2][2];

        CRendererProgram*  m_current;
    };

    buffer_handle_t mTargetHandle;
    uint32_t        m_remainingConstructorAttempts;

    EGLDisplay      m_display; /// Current display id (EGL_NO_DISPLAY if not yet queried from EGL)
    EGLSurface      m_surface; /// Rendering surface (EGL_NO_SURFACE if not yet created)
    EGLContext      m_context; /// Rendering context (EGL_NO_CONTEXT if not yet created)

    bool            m_isFboIdValid; /// Set to false if the FBO is not yet created, true otherwise
    GLuint          m_fboId; /// FBO id if m_isFboIdValid is true, undefined otherwise

    static const uint32_t NumVboIds = 10;

    bool            m_areVboIdsValid; /// Set to false if the VBOs are not yet created, true otherwise
    GLuint          m_vboIds[NumVboIds]; /// VBO ids if m_areVboIdsValid is true, undefined otherwise
    uint32_t        m_nextVboIdIndex;

    CProgramStore   m_programStore;

    // Destination texture
    bool            m_destEGLImageCreated;
    bool            m_destTextureCreated;
    bool            m_destTextureSet;
    uint32_t        m_destWidth;
    uint32_t        m_destHeight;
    android::sp<android::GraphicBuffer> m_destGraphicBuffer;
    EGLImageKHR     m_destEGLImage;
    GLuint          m_destTextureId;
    bool            m_destTextureAttachedToFBO;
    bool            m_nv12TargetSupported;
    bool            m_destIsNV12;

    // Source textures
    uint32_t        m_sourceEGLImagesCreated;
    uint32_t        m_sourceTexturesCreated;
    uint32_t        m_sourceTexturesSet;
    android::sp<android::GraphicBuffer> *m_sourceGraphicBuffers;
    EGLImageKHR*    m_sourceEGLImages;
    GLuint*         m_sourceTextureIds;
    uint32_t        m_maxSourceLayers;

    void bindAVbo();
    bool reallocSourceLayers(uint32_t maxSourceLayers);
    void freeSourceLayers();

    bool verifyContextCreated();

    bool IsLayerNV12(const hwcval_layer_t *pDest);
    bool HasAlpha(const hwcval_layer_t *pSrc);
};

#endif // __HwcTestReferenceComposer_h__
