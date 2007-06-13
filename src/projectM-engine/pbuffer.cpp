/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2004 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */
/**
 * $Id: pbuffer.c,v 1.1.1.1 2005/12/23 18:05:00 psperl Exp $
 *
 * Render target methods
 */

#include <stdio.h>
//#include <GL/gl.h>

#include "common.h"
#include "pbuffer.h"

#ifdef MACOS
#include <agl.h>
#endif /** MACOS */

/** Creates new pbuffers */
void createPBuffers( int width, int height, RenderTarget *target ) {

    int mindim = 0;
    int origtexsize = 0;

    printf( "createPBuffers()\n" );
#ifdef FBO
    printf( "FBO init\n" );
    DWRITE( "FBO init: usePbuffers: %d\n", target->usePbuffers );
    if(target->usePbuffers)
      {
	glewInit();
	
	GLuint   fb, color_rb, depth_rb, rgba_tex, depth_tex, i, other_tex;
	glGenFramebuffersEXT(1, &fb);
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, fb );
	
	glGenRenderbuffersEXT(1, &depth_rb);
	glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, depth_rb );
	glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, target->texsize,target->texsize  );
	glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depth_rb );         
	target->fbuffer[0] = depth_rb;
	
	glGenTextures(1, &other_tex);
	glBindTexture(GL_TEXTURE_2D,other_tex);
 	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, target->texsize, target->texsize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glGenerateMipmapEXT(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);


	
	glGenTextures(1, &rgba_tex);
	glBindTexture(GL_TEXTURE_2D, rgba_tex); 
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, target->texsize, target->texsize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glGenerateMipmapEXT(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);


	
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, rgba_tex, 0 );         
	target->textureID[0] = rgba_tex;
	target->textureID[1] = other_tex; 

	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status == GL_FRAMEBUFFER_COMPLETE_EXT) return;
	else goto fallback;
      }
    
#endif

#ifdef MACOS2
#ifdef MACOSX_10_3
    /** PBuffers are only supported in MacOS X 10.3+ */
    AGLPbuffer pbuffer, pbuffer2;
    AGLPixelFormat pixelFormat;
    AGLContext context;
    GLint attrib[] =
        { AGL_RGBA, AGL_PIXEL_SIZE, 32, AGL_ACCELERATED, AGL_NONE };

    pixelFormat = aglChoosePixelFormat( NULL, 0, attrib );
    if ( pixelFormat == NULL ) {

        goto fallback;
      } else {

      }

    /** Fetch the original context for rendering directly */
    /** Only AGL can be used, otherwise, it'll fall back to texhack */
    /** Try AGL first */
    target->origContext = (void *)aglGetCurrentContext();
    if ( target->origContext == NULL ) {
        /** Try NSGL */
//        target->origContext = (void *)nsglGetCurrentContext();
        target->origContext = NULL;
        if ( target->origContext == NULL ) {
            /** Try CGL */
            target->origContext = (void *)CGLGetCurrentContext();
            if ( target->origContext != NULL ) {
                target->origContext = NULL;
                target->origContextType = CGL_CONTEXT;
              }
          } else {
            target->origContext = NULL;
            target->origContextType = NSGL_CONTEXT;
          }
      } else {
        target->origContextType = AGL_CONTEXT;
      }

    /** 
     * If we can't stash the original context for switching, don't use
     * pbuffers
     */
    if ( target->origContext == NULL ) {

        goto fallback;
      } else {

      }

    context = aglCreateContext( pixelFormat, target->origContext );
    if ( context == NULL ) {

        aglDestroyPixelFormat( pixelFormat );
        goto fallback;
      } else {
        aglDestroyPixelFormat( pixelFormat );

      }

    /** Stash the context and pbuffer */
    target->pbufferContext = (void *)context;

    /** Create the pass1 pbuffer */
    aglCreatePBuffer( target->texsize, target->texsize, GL_TEXTURE_2D,
                      GL_RGBA, 0, &pbuffer );
    if ( pbuffer == NULL ) {

      } else {
        target->pbuffer = pbuffer;

      }

    /** Finally, bind the target texture ID */
    aglSetCurrentContext( target->origContext );
    glGenTextures( 2, &target->textureID[0] );

    glBindTexture( GL_TEXTURE_2D, target->textureID[0] );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

    if ( aglTexImagePBuffer( target->origContext, target->pbuffer, GL_FRONT ) == 0 ) {

      }
    glBindTexture( GL_TEXTURE_2D, 0 );

    glBindTexture( GL_TEXTURE_2D, target->textureID[1] );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D(GL_TEXTURE_2D,
		    0,
		    GL_RGB,
		    target->texsize, target->texsize,
		    0,
		    GL_RGB,
		    GL_UNSIGNED_BYTE,
		    NULL);
    glBindTexture( GL_TEXTURE_2D, 0 );

    glBindTexture( GL_TEXTURE_2D, target->textureID[0] );

    return;

#endif /** MACOSX_10_3 */

    goto fallback;

#else
#endif /** !MACOS */

    /** Successful creation */
    // return;

fallback:
    DWRITE( "using teximage hack fallback\n" );

    /** Fallback pbuffer creation via teximage hack */
    /** Check the texture size against the viewport size */
    /** If the viewport is smaller, then we'll need to scale the texture size down */
    /** If the viewport is larger, scale it up */
    mindim = width < height ? width : height;
    origtexsize = target->texsize;
    target->texsize = nearestPower2( mindim, SCALE_MINIFY );      

    /* Create the texture that will be bound to the render target */
    if ( glIsTexture( target->textureID[0] ) ) {
        DWRITE( "texture already exists\n" );
        if ( target->texsize != origtexsize ) {
            DWRITE( "deleting existing texture due to resize\n" );
            glDeleteTextures( 1, &target->textureID[0] );
          }
      }

    if ( !glIsTexture( target->textureID[0] ) ) {
        glGenTextures(1, &target->textureID[0] );

        DWRITE( "allocate texture: %d\ttexsize: %d x %d\n", 
                target->textureID[0], target->texsize, target->texsize );
        glBindTexture(GL_TEXTURE_2D, target->textureID[0] );
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D,
		    0,
		    3,
		    target->texsize, target->texsize,
		    0,
		    GL_RGB,
		    GL_UNSIGNED_BYTE,
		    NULL);
      }
    return;
  }

/** Destroys the pbuffer */

/** Locks the pbuffer */
void lockPBuffer( RenderTarget *target, PBufferPass pass ) {

#ifdef FBO
  if(target->usePbuffers)
    { 
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, target->fbuffer[0]);     
    }
  
#endif

#ifdef MACOS2
    if ( target->pbufferContext != NULL &&
         target->pbuffer != NULL ) {
        GLint vs;
        DWRITE( "-> locking pbuffer: %d\n", pass );
        if ( !aglSetCurrentContext( (AGLContext)target->pbufferContext ) ) {
            DWRITE( "lockPBuffer(): failed to set context\n" );
          }
        vs = aglGetVirtualScreen ( (AGLContext)target->origContext );
        if ( pass == PBUFFER_PASS1 ) {
            aglSetPBuffer( (AGLContext)target->pbufferContext, (AGLPbuffer)target->pbuffer, 0, 0, vs );
//            glBindTexture( GL_TEXTURE_2D, target->textureID[0] );
          }
      }
#endif
  }

/** Unlocks the pbuffer */
void unlockPBuffer( RenderTarget *target ) {

#ifdef FBO
 
  if(target->usePbuffers)
    {
      glBindTexture( GL_TEXTURE_2D, target->textureID[1] );
      glCopyTexSubImage2D( GL_TEXTURE_2D,
                         0, 0, 0, 0, 0, 
                         target->texsize, target->texsize );
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
      return;
    }

#endif

#ifdef MACOS2
    if ( target->pbufferContext != NULL &&
         target->pbuffer != NULL ) {
        DWRITE( "<- unlocking pbuffer\n" );

        /** Flush the pbuffer */
        glFlush();

        /** Reset the texture ID to the pbuffer */
        glBindTexture( GL_TEXTURE_2D, target->textureID[0] );
        glBindTexture( GL_TEXTURE_2D, 0 );
        glBindTexture( GL_TEXTURE_2D, target->textureID[1] );
        glCopyTexSubImage2D( GL_TEXTURE_2D,
                             0, 0, 0, 0, 0, 
                             target->texsize, target->texsize );
        glBindTexture( GL_TEXTURE_2D, 0 );
        glBindTexture( GL_TEXTURE_2D, target->textureID[0] );

        /** Reset the OpenGL context to the original context */
        aglSetCurrentContext( NULL );
        if ( target->origContext != NULL ) {
            if ( !aglSetCurrentContext( target->origContext ) ) {
                DWRITE( "failed to setting origContext current\n" );
              } else {
                DWRITE( "setting origContext current\n" );
              }
          }


        return;
     }
#endif

    /** Fallback texture path */
    DWRITE( "copying framebuffer to texture\n" );
    glBindTexture( GL_TEXTURE_2D, target->textureID[0] );
    glCopyTexSubImage2D( GL_TEXTURE_2D,
                         0, 0, 0, 0, 0, 
                         target->texsize, target->texsize );
  }

/** 
 * Calculates the nearest power of two to the given number using the
 * appropriate rule
 */
int nearestPower2( int value, TextureScale scaleRule ) {

    int x = value;
    int power = 0;

    DWRITE( "nearestPower2(): %d\n", value );

    while ( ( x & 0x01 ) != 1 ) {
        x >>= 1;
      }

    if ( x == 1 ) {
        return value;
      } else {
        x = value;
        while ( x != 0 ) {
            x >>= 1;
            power++;
          }
        switch ( scaleRule ) {
            case SCALE_NEAREST:
                if ( ( ( 1 << power ) - value ) <= ( value - ( 1 << ( power - 1 ) ) ) ) {
                    return 1 << power;
                  } else {
                    return 1 << ( power - 1 );
                  }
            case SCALE_MAGNIFY:
                return 1 << power;
            case SCALE_MINIFY:
                return 1 << ( power - 1 );
            default:
                break;
          }
      }
    return 0;
  }

