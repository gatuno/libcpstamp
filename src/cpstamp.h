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
	STAMP_TYPE_PIN
};

enum {
	STAMP_EASY = 0,
	STAMP_NORMAL,
	STAMP_HARD,
	STAMP_EXTREME
};

typedef struct Stamp Stamp;

typedef struct Categoria Categoria;

extern SDL_Rect stamp_rect;

int iniciarCPStamp (void);
Categoria *abrir_cat (int tipo, char *nombre, char *clave);

Categoria *crear_cat (int tipo, char *nombre, char *clave);
void registrar_estampa (Categoria *cat, int id, char *titulo, int categoria, int dificultad);
void cerrar_registro (Categoria *cat);
void earn_stamp (Categoria *c, int id);

void restaurar_dibujado (SDL_Surface *screen);
void dibujar_estampa (SDL_Surface *screen, Categoria *c, int save);

#endif /* __CP_STAMP_H__ */

