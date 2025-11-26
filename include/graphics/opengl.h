#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <gl/gl.h>

#include <opengl/glext.h>
#define GL_GET_PROC_ADDRESS wglGetProcAddress
#else
#include <GL/gl.h>
#include <GL/glext.h>
#define GL_GET_PROC_ADDRESS glfwGetProcAddress
#endif

#define FOR_OPENGL_FUNCTIONS(DO)                                                              \
    DO(::PFNGLCREATESHADERPROC, glCreateShader)                                               \
    DO(::PFNGLDELETESHADERPROC, glDeleteShader)                                               \
    DO(::PFNGLSHADERSOURCEPROC, glShaderSource)                                               \
    DO(::PFNGLCOMPILESHADERARBPROC, glCompileShader)                                          \
    DO(::PFNGLGETSHADERIVPROC, glGetShaderiv)                                                 \
    DO(::PFNGLCREATEPROGRAMPROC, glCreateProgram)                                             \
    DO(::PFNGLDELETEPROGRAMPROC, glDeleteProgram)                                             \
    DO(::PFNGLATTACHSHADERPROC, glAttachShader)                                               \
    DO(::PFNGLLINKPROGRAMPROC, glLinkProgram)                                                 \
    DO(::PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays)                                   \
    DO(::PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays)                                         \
    DO(::PFNGLGENBUFFERSPROC, glGenBuffers)                                                   \
    DO(::PFNGLDELETEBUFFERSPROC, glDeleteBuffers)                                             \
    DO(::PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)                                         \
    DO(::PFNGLBINDBUFFERPROC, glBindBuffer)                                                   \
    DO(::PFNGLBUFFERDATAPROC, glBufferData)                                                   \
    DO(::PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer)                                 \
    DO(::PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)                         \
    DO(::PFNGLUSEPROGRAMPROC, glUseProgram)                                                   \
    DO(::PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)                                       \
    DO(::PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)                                     \
    DO(::PFNGLGETPROGRAMIVPROC, glGetProgramiv)                                               \
    DO(::PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback)                               \
    DO(::PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation)                                   \
    DO(::PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv)                                       \
    DO(::PFNGLGETACTIVEUNIFORMPROC, glGetActiveUniform)                                       \
    DO(::PFNGLUNIFORM1IPROC, glUniform1i)                                                     \
    DO(::PFNGLUNIFORM1FPROC, glUniform1f)                                                     \
    DO(::PFNGLUNIFORM3FVPROC, glUniform3fv)                                                   \
    DO(::PFNGLCREATEBUFFERSPROC, glCreateBuffers)                                             \
    DO(::PFNGLNAMEDBUFFERSTORAGEPROC, glNamedBufferStorage)                                   \
    DO(::PFNGLCREATEVERTEXARRAYSPROC, glCreateVertexArrays)                                   \
    DO(::PFNGLVERTEXARRAYVERTEXBUFFERPROC, glVertexArrayVertexBuffer)                         \
    DO(::PFNGLENABLEVERTEXARRAYATTRIBPROC, glEnableVertexArrayAttrib)                         \
    DO(::PFNGLVERTEXARRAYATTRIBFORMATPROC, glVertexArrayAttribFormat)                         \
    DO(::PFNGLNAMEDBUFFERSUBDATAPROC, glNamedBufferSubData)                                   \
    DO(::PFNGLVERTEXARRAYELEMENTBUFFERPROC, glVertexArrayElementBuffer)                       \
    DO(::PFNGLVERTEXARRAYATTRIBBINDINGPROC, glVertexArrayAttribBinding)                       \
    DO(::PFNGLBINDBUFFERBASEPROC, glBindBufferBase)                                           \
    DO(::PFNGLCREATETEXTURESPROC, glCreateTextures)                                           \
    DO(::PFNGLTEXTURESTORAGE2DPROC, glTextureStorage2D)                                       \
    DO(::PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC, glTextureStorage2DMultisample)                 \
    DO(::PFNGLTEXTURESUBIMAGE2DPROC, glTextureSubImage2D)                                     \
    DO(::PFNGLTEXTURESTORAGE3DPROC, glTextureStorage3D)                                       \
    DO(::PFNGLTEXTURESUBIMAGE3DPROC, glTextureSubImage3D)                                     \
    DO(::PFNGLCREATESAMPLERSPROC, glCreateSamplers)                                           \
    DO(::PFNGLDELETESAMPLERSPROC, glDeleteSamplers)                                           \
    DO(::PFNGLBINDTEXTUREUNITPROC, glBindTextureUnit)                                         \
    DO(::PFNGLBINDSAMPLERPROC, glBindSampler)                                                 \
    DO(::PFNGLSAMPLERPARAMETERIPROC, glSamplerParamerteri)                                    \
    DO(::PFNGLSAMPLERPARAMETERFPROC, glSamplerParamerterf)                                    \
    DO(::PFNGLCREATEFRAMEBUFFERSPROC, glCreateFramebuffers)                                   \
    DO(::PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers)                                   \
    DO(::PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer)                                         \
    DO(::PFNGLNAMEDFRAMEBUFFERTEXTUREPROC, glNamedFramebufferTexture)                         \
    DO(::PFNGLBLITNAMEDFRAMEBUFFERPROC, glBlitNamedFramebuffer)                               \
    DO(::PFNGLGENERATETEXTUREMIPMAPPROC, glGenerateTextureMipmap)                             \
    DO(::PFNGLCREATERENDERBUFFERSPROC, glCreateRenderbuffers)                                 \
    DO(::PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC, glNamedFramebufferDrawBuffers)                 \
    DO(::PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC, glNamedFramebufferDrawBuffer)                   \
    DO(::PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC, glNamedFramebufferRenderbuffer)               \
    DO(::PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC, glNamedFramebufferReadBuffer)                   \
    DO(::PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC, glNamedRenderbufferStorageMultisample) \
    DO(::PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC, glCheckNamedFramebufferStatus)                 \
    DO(::PFNGLDRAWARRAYSEXTPROC, glDrawArraysExt)

#define DO_INLINE(TYPE, NAME) inline TYPE NAME;
FOR_OPENGL_FUNCTIONS(DO_INLINE)
