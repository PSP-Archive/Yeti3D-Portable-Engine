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

/*
** Name: Yeti3D
** Desc: Portable GameBoy Advanced 3D Engine
** Auth: Derek J. Evans <derek@theteahouse.com.au>
**
** Copyright (C) 2003 Derek J. Evans. All Rights Reserved.
**
** YY  YY EEEEEE TTTTTT IIIIII 33333  DDDDD
** YY  YY EE       TT     II       33 DD  DD
**  YYYY  EEEE     TT     II     333  DD  DD
**   YY   EE       TT     II       33 DD  DD
**   YY   EEEEEE   TT   IIIIII 33333  DDDDD
*/

#include <proto/exec.h>
#include <proto/picasso96api.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <devices/keyboard.h>

/* There is a conflict with MIN & MAX macros but it's basicly the same definition */
#include <time.h>

// turhaa?
#include "../../src/yeti.h"
#include "../../src/game.h"

struct Library* GfxBase = NULL;
struct Library* IntuitionBase = NULL;
struct Library* P96Base = NULL;
struct Library* DosBase = NULL;

struct GraphicsIFace* IGraphics = NULL;
struct IntuitionIFace* IIntuition = NULL;
struct P96IFace* IP96 = NULL;

/* Yeti window */
struct Window* pWindow = NULL;

/* Buffer */
struct BitMap * pBitMap = NULL;

/* Window's rastport */
struct RastPort* pRastPort = NULL;

/* Picasso96 render info */
struct RenderInfo ri;

/* Keyboard reading stuff */
struct Device * pDevice;
struct MsgPort * pMsgPort;
struct IOStdReq * pRequest;
UBYTE matrix[16] = { 0 };

int frames = 0;

framebuffer_t framebuffer;
yeti_t yeti;

/* Update keyboard matrix */
void keyboard_update( keyboard_t * );

/* Draw frame */
void flip(void)
{
//  static unsigned MarkTime;

//  if ((int)(MarkTime - timeGetTime()) < 0)
  {
//    MarkTime = timeGetTime() + YETI_VIEWPORT_INTERVAL;   

    keyboard_update(&yeti.keyboard);
    game_loop(&yeti);

	ULONG lock = IP96->p96LockBitMap( pBitMap, (UBYTE*)&ri, sizeof(struct RenderInfo) );

	if (lock)
    {
/*		int x, y;
    	u16* fb = (u16*) ri.Memory;

  		for (y = 0; y < YETI_VIEWPORT_HEIGHT; y++)
  		{
			u16* _fb = fb + y * ri.BytesPerRow/2;
   			rgb555_t* tb= yeti.viewport.back->pixels[y];
	
			for (x = 0; x < YETI_VIEWPORT_WIDTH; x++)
			{		
				*_fb++ = yeti.viewport.video_lut[*tb++];
			}
		}
*/
      	viewport_to_video(
        	(u16*) ri.Memory,
        	ri.BytesPerRow,
        	&yeti.viewport,
        	0x7C01, 0x3E0, 0x1F
		);

		IP96->p96UnlockBitMap( pBitMap, lock );
		IGraphics->WaitTOF();
		IGraphics->BltBitMapRastPort( pBitMap, 0, 0, 
			pRastPort, pWindow->BorderLeft, pWindow->BorderTop,
			YETI_VIEWPORT_WIDTH, YETI_VIEWPORT_HEIGHT, 0xC0 );

		frames++;
    }
  }
}


/* Allocate resources */
BOOL init_resources()
{
	// Open libraries
	GfxBase = IExec->OpenLibrary("graphics.library", 50);
	IntuitionBase = IExec->OpenLibrary("intuition.library", 50);
	P96Base = IExec->OpenLibrary("Picasso96API.library", 0);
	DosBase = IExec->OpenLibrary("dos.library", 50);

	if ( !( GfxBase && IntuitionBase && P96Base && DosBase ) )
		return FALSE;

	// Get interfaces 
	IGraphics = (struct GraphicsIFace*)IExec->GetInterface(GfxBase, "main", 1, NULL);
	IIntuition = (struct IntuitionIFace*)IExec->GetInterface(IntuitionBase, "main", 1, NULL);
	IP96 = (struct P96IFace*)IExec->GetInterface(P96Base, "main", 1, NULL);
	IDOS = (struct IDOS*)IExec->GetInterface(DosBase, "main", 1, NULL);

	/* Create message port for keyboard reading */
	if ( ! ( pMsgPort = IExec->CreateMsgPort() ) ) return FALSE;

	/* Create request for keyboard device I/O */
	if ( ! ( pRequest = (struct IOStdReq*)IExec->CreateIORequest( pMsgPort, sizeof(struct IOStdReq) ) ) ) return FALSE;

	/* OpenDevice returns 0 if success! */
	if ( ( IExec->OpenDevice( "keyboard.device", 0, (struct IORequest*) pRequest, 0 ) )) return FALSE;
	
	if ( !(IGraphics && IIntuition && IP96 && IDOS ) )
		return FALSE;

	return TRUE;
}


/* Deallocate & close bitmaps, windows, libraries */
void clean()
{
	if (pWindow)
	{
		if (IIntuition) IIntuition->CloseWindow(pWindow);
	}

	if (pBitMap)
	{
		if (IGraphics) IGraphics->WaitBlit();
		if (IP96) IP96->p96FreeBitMap( pBitMap );
	}

	if (IP96) IExec->DropInterface( (struct Interface*) IP96 );
	if (IGraphics) IExec->DropInterface( (struct Interface*) IGraphics );
	if (IIntuition) IExec->DropInterface( (struct Interface*) IIntuition );
	if (IDOS) IExec->DropInterface( (struct Interface*) IDOS );

	if (P96Base) IExec->CloseLibrary( P96Base );
	if (GfxBase) IExec->CloseLibrary( GfxBase );
	if (IntuitionBase) IExec->CloseLibrary( IntuitionBase );
	if (DosBase) IExec->CloseLibrary( DosBase );

	if ( pRequest )
	{
		IExec->CloseDevice( (struct IORequest*) pRequest );
		IExec->DeleteIORequest( (struct IORequest*) pRequest );
	}	
	
	if ( pMsgPort ) IExec->DeleteMsgPort( pMsgPort );
}


/* Update keyboard matrix */
void keyboard_update(keyboard_t * kb)
{
	ULONG bActive;

	IIntuition->GetWindowAttrs( pWindow, WA_Activate, &bActive, sizeof(ULONG) ); 

	if (!bActive)
	{
		kb->up = kb->down = kb->left = kb->right = kb->b = kb->a = kb->l = kb->r = 0;
		return;
	}
	pRequest->io_Command = KBD_READMATRIX;
    pRequest->io_Data = (APTR)&matrix;
	pRequest->io_Length = 16;

    IExec->DoIO( (struct IORequest *) pRequest );

/*
	{
		int i;
		for ( i = 0; i < 16; i++ )
		{
			printf("%x:", matrix[i] );
		}
		printf("\n");
	}
*/
	/* There is probably better way... */
	kb->up = matrix[9] & 0x10; /* cursor up */ 
	kb->down = matrix[9] & 0x20;
	kb->left = matrix[9] & 0x80;
	kb->right = matrix[9] & 0x40;
	kb->b = matrix[8] & 1; /* space */
	kb->l = matrix[4] & 1; /* a */
	kb->r = matrix[6] & 2; /* z */
	kb->a = matrix[12] & 8; /* control */
}


/* Handle possible messages */
void event_loop(void)
{
	BOOL quit = FALSE;
	struct MsgPort * port = pWindow->UserPort;
	struct IntuiMessage * msg;
	LONG argv;

	ULONG start = time( NULL );

	while (!quit)
	{
		while ( (msg = (struct IntuiMessage*) IExec->GetMsg( port )) )
		{
			ULONG class = msg->Class;

			IExec->ReplyMsg( (struct Message*) msg );

			switch( class )
			{
			case CLOSEWINDOW:
				quit = TRUE;
				break;

			default:
				break;
			}
		}
		
		/* Draw frame */
		flip();
	}

	argv = frames / (time( NULL ) - start);
	IDOS->VPrintf("FPS: ~%ld\n", &argv);
}


/* Main program */
int main(void)
{
	struct Screen * pScr;

	if ( !init_resources() )
	{
		clean();
		return(0);
	}

	pBitMap = IP96->p96AllocBitMap( YETI_VIEWPORT_WIDTH, YETI_VIEWPORT_HEIGHT, 16, BMF_CLEAR, NULL, RGBFB_R5G5B5 );

	pScr = IIntuition->LockPubScreen( NULL );
	if (!pScr) return FALSE;

	pWindow = IIntuition->OpenWindowTags(
		NULL,
		WA_Title, "Yeti",
		WA_PubScreen, pScr,
		WA_InnerWidth, YETI_VIEWPORT_WIDTH,
		WA_InnerHeight, YETI_VIEWPORT_HEIGHT,
		WA_IDCMP, CLOSEWINDOW,
		WA_CloseGadget, TRUE,
		WA_DragBar, TRUE,
		WA_DepthGadget, TRUE,
		TAG_END );

	IIntuition->UnlockPubScreen( NULL, pScr );

	if (pWindow && pBitMap)
	{
		pRastPort = pWindow->RPort;

        yeti_init(&yeti, &framebuffer, &framebuffer, textures, palette, lua);
        game_init(&yeti);

/*
		if (!yeti.viewport.video_lut_filled)
  		{
			int i;
    		yeti.viewport.video_lut_filled = TRUE;

		    for (i = RGB_MAX; i--;)
    		{
      			yeti.viewport.video_lut[i] = rgb_convert(i, 0x7C01, 0x3E0, 0x1F);
    		}
		}
*/
		event_loop();
	}

	clean();

	return 0;
}
