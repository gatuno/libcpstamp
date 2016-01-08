/*
 * cpstamp.h
 * This file is part of LibCPStamp
 *
 * Copyright (C) 2014 - Félix Arreola Rodríguez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CP_STAMP_H__
#define __CP_STAMP_H__

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

/* Define las posibles categorías para la estampa */
enum {
	STAMP_TYPE_ACTIVITY = 0,
	STAMP_TYPE_GAME,
	STAMP_TYPE_EVENT,
	STAMP_TYPE_PIN,
	
	NUM_STAMP_TYPE
};

enum {
	STAMP_EASY = 0,
	STAMP_NORMAL,
	STAMP_HARD,
	STAMP_EXTREME
};

typedef struct CPStampCategory CPStampCategory;

extern SDL_Rect cpstamp_rect;
extern int cpstamp_activate;

int CPStamp_Init (int argc, char **argv);

CPStampCategory *CPStamp_Open (int tipo, char *nombre, char *clave);
void CPStamp_Close (CPStampCategory *cat);

void CPStamp_Register (CPStampCategory *cat, int id, char *titulo, char *descripcion, char *imagen, int categoria, int dificultad);
int CPStamp_IsRegistered (CPStampCategory *cat, int id);
void CPStamp_Earn (CPStampCategory *cat, int id);

void CPStamp_Restore (SDL_Surface *screen);
void CPStamp_Draw (SDL_Surface *screen, CPStampCategory *cat, int save);

void CPStamp_ClearStamps (CPStampCategory *cat);

#endif /* __CP_STAMP_H__ */

