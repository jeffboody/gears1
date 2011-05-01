/*
 * Copyright (c) 2009-2010 Jeff Boody
 * Gears for Android(TM) is a heavily modified port of the infamous "gears" demo to
 * Android/Java/GLES 1.0. As such, it is a derived work subject to the license
 * requirements (below) of the original work.
 *
 * Copyright (c) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "gears_renderer.h"
#include "a3d/a3d_time.h"

#define LOG_TAG "gears"
#include "a3d/a3d_log.h"

/***********************************************************
* private                                                  *
***********************************************************/

// gear colors
static const a3d_vec4f_t RED   = { 0.8f, 0.1f,  0.0f, 1.0f };
static const a3d_vec4f_t GREEN = { 0.0f, 0.8f,  0.2f, 1.0f };
static const a3d_vec4f_t BLUE  = { 0.2f, 0.2f,  1.0f, 1.0f };

// light position
static const GLfloat LIGHT_POSITION[] = { 5.0f, 5.0f, 10.0f, 0.0f };

static void gears_renderer_step(gears_renderer_t* self)
{
	assert(self);
	LOGD("debug");

	double t     = a3d_utime();
	double dt0   = t - self->t0;
	++self->frames;

	// don't update fps every frame
	if(dt0 >= 1.0 * A3D_USEC)
	{
		double seconds = dt0 / A3D_USEC;
		double fps     = (double) self->frames / seconds;

		// LOGI("%i frames in %.2lf seconds = %.2lf FPS", self->frames, seconds, fps);
		a3d_texstring_printf(self->fps, "%i fps", (int) fps);

		self->t0     = t;
		self->frames = 0;
	}
	
	// next frame
	if(self->t_last > 0.0)
	{
		float dt_last = (float) ((t - self->t_last) / A3D_USEC);
	
		// make the gears rotate at a constant rate
		// red gear rotates at 1 revolution per 7 seconds
		self->angle += 360.0f * dt_last / 7.0f;
	}
	self->t_last = t;
}

/***********************************************************
* public                                                   *
***********************************************************/

gears_renderer_t* gears_renderer_new(const char* font)
{
	LOGD("debug");

	LOGI("GL vendor     : %s", glGetString(GL_VENDOR));
	LOGI("GL renderer   : %s", glGetString(GL_RENDERER));
	LOGI("GL version    : %s", glGetString(GL_VERSION));
	LOGI("GL extensions : %s", glGetString(GL_EXTENSIONS));

	gears_renderer_t* self = (gears_renderer_t*) malloc(sizeof(gears_renderer_t));
	if(self == NULL)
	{
		LOGE("malloc failed");
		return NULL;
	}

	// create the gears
	self->gear1 = gear_new(&RED, 1.0f, 4.0f, 1.0f, 20, 0.7f);
	if(self->gear1 == NULL) goto fail_gear1;

	self->gear2 = gear_new(&GREEN, 0.5f, 2.0f, 2.0f, 10, 0.7f);
	if(self->gear2 == NULL) goto fail_gear2;

	self->gear3 = gear_new(&BLUE, 1.3f, 2.0f, 0.5f, 10, 0.7f);
	if(self->gear3 == NULL) goto fail_gear3;

	// create the fps counter
	self->font = a3d_texfont_new(font);
	if(self->font == NULL)
		goto fail_font;

	self->fps = a3d_texstring_new(self->font, 16, 48, A3D_TEXSTRING_BOTTOM_RIGHT, 1.0f, 1.0f, 0.235f, 1.0f);
	if(self->fps == NULL)
		goto fail_fps;

	// create the mutex
	// PTHREAD_MUTEX_DEFAULT is not re-entrant
	if(pthread_mutex_init(&self->mutex, NULL) != 0)
	{
		LOGE("pthread_mutex_init failed");
		goto fail_mutex_init;
	}

	// initialize state
	self->view_scale = 1.0f;
	self->view_rot_x = 20.0f;
	self->view_rot_y = 30.0f;
	self->view_rot_z = 0.0f;
	self->angle      = 0.0f;
	self->t0         = a3d_utime();
	self->t_last     = 0.0;
	self->frames     = 0;

	glLightfv(GL_LIGHT0, GL_POSITION, LIGHT_POSITION);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glEnableClientState(GL_VERTEX_ARRAY);

	// success
	return self;

	// failure
	fail_mutex_init:
		a3d_texstring_delete(&self->fps);
	fail_fps:
		a3d_texfont_delete(&self->font);
	fail_font:
		gear_delete(&self->gear3);
	fail_gear3:
		gear_delete(&self->gear2);
	fail_gear2:
		gear_delete(&self->gear1);
	fail_gear1:
		free(self);
	return NULL;
}

void gears_renderer_delete(gears_renderer_t** _self)
{
	// *_self can be null
	assert(_self);

	gears_renderer_t* self = *_self;
	if(self)
	{
		LOGD("debug");

		pthread_mutex_destroy(&self->mutex);
		a3d_texstring_delete(&self->fps);
		a3d_texfont_delete(&self->font);
		gear_delete(&self->gear3);
		gear_delete(&self->gear2);
		gear_delete(&self->gear1);

		free(self);
		*_self = NULL;
	}
}

void gears_renderer_resize(gears_renderer_t* self, GLsizei w, GLsizei h)
{
	assert(self);
	LOGI("%ix%i", w, h);

	self->w = w;
	self->h = h;
	glViewport(0, 0, w, h);
}

void gears_renderer_rotate(gears_renderer_t* self, float dx, float dy)
{
	assert(self);
	LOGD("debug dx=%f, dy=%f", dx, dy);

	if(pthread_mutex_lock(&self->mutex) != 0)
		LOGE("pthread_mutex_lock failed");

	// rotating around x-axis is equivalent to moving up-and-down on touchscreen
	// rotating around y-axis is equivalent to moving left-and-right on touchscreen
	// 360 degrees is equivalent to moving completly across the touchscreen
	float w = (float) self->w;
	float h = (float) self->h;
	self->view_rot_x += 360.0f * dy / h;
	self->view_rot_y += 360.0f * dx / w;

	if(pthread_mutex_unlock(&self->mutex) != 0)
		LOGE("pthread_mutex_unlock failed");
}

void gears_renderer_scale(gears_renderer_t* self, float ds)
{
	assert(self);
	LOGD("debug ds=%f", ds);

	if(pthread_mutex_lock(&self->mutex) != 0)
		LOGE("pthread_mutex_lock failed");

	// scale range
	float min = 0.25f;
	float max = 2.0f;

	float w = (float) self->w;
	float h = (float) self->h;
	self->view_scale -= ds / sqrtf(w*w + h*h);
	if(self->view_scale < min)  self->view_scale = min;
	if(self->view_scale >= max) self->view_scale = max;

	if(pthread_mutex_unlock(&self->mutex) != 0)
		LOGE("pthread_mutex_unlock failed");
}

void gears_renderer_draw(gears_renderer_t* self)
{
	assert(self);
	LOGD("debug");

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// for fps text ...
	GLfloat w = (float) self->w;
	GLfloat h = (float) self->h;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if(h > w)
	{
		GLfloat a = h / w;
		glFrustumf(-1.0f, 1.0f, -a, a, 5.0f, 60.0f);
	}
	else
	{
		GLfloat a = w / h;
		glFrustumf(-a, a, -1.0f, 1.0f, 5.0f, 60.0f);
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -40.0f);
	glLightfv(GL_LIGHT0, GL_POSITION, LIGHT_POSITION);

	// glxgears: event_loop
	glPushMatrix();
	if(pthread_mutex_lock(&self->mutex) != 0)
		LOGE("pthread_mutex_lock failed");
	glScalef(self->view_scale, self->view_scale, self->view_scale);
	glRotatef(self->view_rot_x, 1.0f, 0.0f, 0.0f);
	glRotatef(self->view_rot_y, 0.0f, 1.0f, 0.0f);
	glRotatef(self->view_rot_z, 0.0f, 0.0f, 1.0f);
	if(pthread_mutex_unlock(&self->mutex) != 0)
		LOGE("pthread_mutex_unlock failed");

	// Gear1
	glPushMatrix();
	glTranslatef(-3.0f, -2.0f, 0.0f);
	glRotatef(self->angle, 0.0f, 0.0f, 1.0f);
	gear_draw(self->gear1);
	glPopMatrix();

	// Gear2
	glPushMatrix();
	glTranslatef(3.1f, -2.0f, 0.0f);
	glRotatef(-2.0f * self->angle - 9.0f, 0.0f, 0.0f, 1.0f);
	gear_draw(self->gear2);
	glPopMatrix();

	// Gear3
	glPushMatrix();
	glTranslatef(-3.1f, 4.2f, 0.0f);
	glRotatef(-2.0f * self->angle - 25.0f, 0.0f, 0.0f, 1.0f);
	gear_draw(self->gear3);
	glPopMatrix();

	glPopMatrix();

	gears_renderer_step(self);

	// draw fps
	glDisable(GL_LIGHTING);
	a3d_texstring_draw(self->fps, (float) self->w - 16.0f, (float) self->h - 16.0f, self->w, self->h);
	glEnable(GL_LIGHTING);

	A3D_GL_GETERROR();
}
