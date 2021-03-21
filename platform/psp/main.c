/*
Copyright (C) 2003 - Derek John Evans 

This file is part of Yeti3D Portable Engine

Yeti3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 2003 - Derek J. Evans <derek@theteahouse.com.au>
Prepared for public release: 10/24/2003 - Derek J. Evans <derek@theteahouse.com.au>
*/

#define YETI_PSP

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "../../src/yeti.h"
#include "../../src/font.h"
#include "../../src/game.h"

u16* vram_ptr = (u16*) 0x44000000;
IN_EWRAM yeti_t yeti;
framebuffer_t* yeti_front;
framebuffer_t* yeti_back;
SceCtrlData pad;
int running = 1;

PSP_MODULE_INFO("Yeti3D", 0, 1, 1);

int ExitCallback(int arg1, int arg2, void *common)
{
	running = 0;
	return 0;
}

int CallbackThread(SceSize args, void *argp)
{
	int cbid;
	cbid = sceKernelCreateCallback("Exit Callback", ExitCallback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

int SetupCallbacks(void)
{
	int thid = 0;
	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
		sceKernelStartThread(thid, 0, 0);
	return thid;
}

void psp_controls(void)
{
	//check controls...
	sceCtrlPeekBufferPositive(&pad, 1);
	if(pad.Buttons & PSP_CTRL_CROSS)
		{yeti.keyboard.a = 0xFF;}
	else
		{yeti.keyboard.a = 0x00;}
	if(pad.Buttons & PSP_CTRL_CIRCLE)
		{yeti.keyboard.b = 0xFF;}
	else
		{yeti.keyboard.b = 0x00;}
	if(pad.Buttons & PSP_CTRL_SELECT)
		{yeti.keyboard.select = 0xFF;}
	else
		{yeti.keyboard.select = 0x00;}
	if(pad.Buttons & PSP_CTRL_RIGHT)
		{yeti.keyboard.right = 0xFF;}
	else
		{yeti.keyboard.right = 0x00;}
	if(pad.Buttons & PSP_CTRL_LEFT)
		{yeti.keyboard.left = 0xFF;}
	else
		{yeti.keyboard.left = 0x00;}
	if(pad.Buttons & PSP_CTRL_UP)
		{yeti.keyboard.up = 0xFF;}
	else
		{yeti.keyboard.up = 0x00;}
	if(pad.Buttons & PSP_CTRL_DOWN)
		{yeti.keyboard.down = 0xFF;}
	else
		{yeti.keyboard.down = 0x00;}
	if(pad.Buttons & PSP_CTRL_LTRIGGER)
		{yeti.keyboard.l = 0xFF;}
	else
		{yeti.keyboard.l = 0x00;}
	if(pad.Buttons & PSP_CTRL_RTRIGGER)
		{yeti.keyboard.r = 0xFF;}
	else
		{yeti.keyboard.r = 0x00;}
}

void psp_flip(void)
{
	//wait for vblank
	sceDisplayWaitVblankStart();

	//swap framebuffers
	framebuffer_t* temp;
	temp = yeti.viewport.front;
	yeti.viewport.front = yeti.viewport.back;
	yeti.viewport.back = temp;
	
	//write to screen
	u16* screen = vram_ptr;
	int x, y;
	for(y = 0; y < YETI_VIEWPORT_HEIGHT; y++)
	{
		for(x = 0; x < YETI_VIEWPORT_WIDTH; x++)
		{
			screen[(y * 512)  + x] = yeti.viewport.front->pixels[y][x];
		}
	}
}

//main
int main(void)
{
	//setup callbacks
	SetupCallbacks();
	
	//setup screen
	pspDebugScreenInitEx((void*)vram_ptr, PSP_DISPLAY_PIXEL_FORMAT_5551, 1);
	
	//allocate space for screens
	yeti_front = (framebuffer_t*)malloc(sizeof(framebuffer_t));
	yeti_back = (framebuffer_t*)malloc(sizeof(framebuffer_t));
	
	//init
	yeti_init(&yeti, yeti_front, yeti_back, textures, palette, lua);
	game_init(&yeti);
	psp_controls();

	//loop
	while(running)
	{
		game_tick(&yeti);
		game_draw(&yeti);

		//for(real = 0; real < YETI_VIEWPORT_INTERVAL; real += YETI_VIEWPORT_INTERVAL_ERROR)
		//{
			//game_tick(&yeti);
		//}

		psp_controls();
		psp_flip();
	}
	sceKernelExitGame();
	return 0;
}
