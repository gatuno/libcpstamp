/*
 * cpstamp.c
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

#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#ifdef __MINGW32__
#include <windows.h>
#include <shellapi.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <locale.h>
#include "gettext.h"
#define _(string) gettext (string)

#include "cpstamp.h"
#include "path.h"

typedef struct CPStamp CPStamp;

struct CPStamp {
	int id;
	char *titulo;
	char *descripcion;
	char *imagen;
	
	int categoria;
	int dificultad;
	
	int ganada;
	
	CPStamp *sig;
};

struct CPStampCategory {
	char *nombre;
	int tipo;
	
	CPStamp *lista;
	
	int fd;
};

enum {
	IMG_STAMP_PANEL,
	
	IMG_STAMP_GAME_EASY,
	IMG_STAMP_GAME_NORMAL,
	IMG_STAMP_GAME_HARD,
	IMG_STAMP_GAME_EXTREME,
	
	NUM_IMGS
};

const char * stamp_images_names [NUM_IMGS] = {
	"images/panel_stamp.png",
	
	"images/game_easy.png",
	"images/game_normal.png",
	"images/game_hard.png",
	"images/game_extreme.png"
};

SDL_Surface *stamp_images [NUM_IMGS];
SDL_Surface *save_screen;
SDL_Surface *stamp_earned[2];

TTF_Font *font;

Mix_Chunk *stamp_sound_earn;

int stamp_queue[10];
int stamp_queue_start, stamp_queue_end;
int stamp_timer;

SDL_Rect cpstamp_rect;
int cpstamp_activate;

#ifndef MAX_PATH
#	define MAX_PATH 2048
#endif
char config_directory[MAX_PATH];

int CPStamp_Init (int argc, char **argv) {
	int g, h;
	SDL_Color blanco, negro;
	char buffer_file[8192];
	
	/* Conseguir las urls del sistema */
	initSystemLibPaths (argv[0]);
	
	/* Inicializar i18n */
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, l10nlib_path);
	
	textdomain (PACKAGE);
	
	for (g = 0; g < NUM_IMGS; g++) {
		sprintf (buffer_file, "%s%s", systemdatalib_path, stamp_images_names [g]);
		stamp_images[g] = IMG_Load (buffer_file);
		
		if (stamp_images[g] == NULL) {
			for (h = 0; h < g; h++) {
				SDL_FreeSurface (stamp_images[h]);
				stamp_images[h] = NULL;
			}
			
			return -1;
		}
	}
	
	save_screen = SDL_AllocSurface (SDL_SWSURFACE, stamp_images[IMG_STAMP_PANEL]->w, stamp_images[IMG_STAMP_PANEL]->h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	
	sprintf (buffer_file, "%ssounds/earn.wav", systemdatalib_path);
	stamp_sound_earn = Mix_LoadWAV (buffer_file);
	
	cpstamp_activate = stamp_timer = stamp_queue_start = stamp_queue_end = 0;
	
	if (!TTF_WasInit ()) {
		TTF_Init ();
	}
	
	if (TTF_WasInit ()) {
		sprintf (buffer_file, "%sburbanksb.ttf", systemdatalib_path);
		font = TTF_OpenFont (buffer_file, 11);
		
		if (font != NULL) {
			blanco.r = blanco.g = blanco.b = 255;
			negro.r = negro.g = negro.b = 0;
			
			stamp_earned[0] = TTF_RenderUTF8_Blended (font, _("Stamp Earned!"), negro);
			stamp_earned[1] = TTF_RenderUTF8_Blended (font, _("Stamp Earned!"), blanco);
		} else {
			stamp_earned [0] = stamp_earned [1] = NULL;
		}
	} else {
		font = NULL;
		stamp_earned [0] = stamp_earned [1] = NULL;
	}
}

void CPStamp_Earn (CPStampCategory *cat, int id) {
	CPStamp *s;
	
	if (cat == NULL) return;
	s = cat->lista;
	
	while (s != NULL) {
		if (s->id == id) {
			if (!s->ganada) {
				cpstamp_activate = 1;
				s->ganada = TRUE;
				stamp_queue [stamp_queue_end] = id;
				stamp_queue_end = (stamp_queue_end + 1) % 10;
			}
			break;
		}
		
		s = s->sig;
	}
}

void CPStamp_Draw (SDL_Surface *screen, CPStampCategory *cat, int save) {
	SDL_Rect rect;
	int imagen;
	static CPStamp *local;
	static SDL_Surface *subtext[2];
	
	if (cat == NULL) return;
	if (stamp_queue_start == stamp_queue_end) {
		cpstamp_activate = 0;
		return;
	}
	
	if (stamp_timer == 0) {
		SDL_Color blanco, negro;
		/* Encontrar la estampa a mostrar */
		local = cat->lista;
		
		while (local != NULL) {
			if (local->id == stamp_queue[stamp_queue_start]) {
				break;
			}
			
			local = local->sig;
		}
		
		if (font != NULL) {
			blanco.r = blanco.g = blanco.b = 255;
			negro.r = negro.g = negro.b = 0;
			
			subtext[0] = TTF_RenderUTF8_Blended (font, local->titulo, negro);
			subtext[1] = TTF_RenderUTF8_Blended (font, local->titulo, blanco);
		}
	}
	
	if (stamp_timer >= 8 && save) {
		cpstamp_rect.x = 392; cpstamp_rect.y = 0;
		cpstamp_rect.w = stamp_images[IMG_STAMP_PANEL]->w;
		cpstamp_rect.h = stamp_images[IMG_STAMP_PANEL]->h;
		
		SDL_BlitSurface (screen, &cpstamp_rect, save_screen, NULL);
	}
	
	if (stamp_timer < 56) {
		cpstamp_rect.h = stamp_images[IMG_STAMP_PANEL]->h;
		cpstamp_rect.w = stamp_images[IMG_STAMP_PANEL]->w;
		cpstamp_rect.x = 392;
		switch (stamp_timer) {
			case 8:
				cpstamp_rect.y = 20 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 9:
				cpstamp_rect.y = 40 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 10:
				cpstamp_rect.y = 53 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 11:
				Mix_PlayChannel (-1, stamp_sound_earn, 0);
				cpstamp_rect.y = 66 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 12:
				cpstamp_rect.y = 72 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 13:
				cpstamp_rect.y = 78 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 52:
				cpstamp_rect.y = 77 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 53:
				cpstamp_rect.y = 67 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 54:
				cpstamp_rect.y = 51 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 55:
				cpstamp_rect.y = 29 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
		}
		
		if (stamp_timer > 13 && stamp_timer < 52) {
			cpstamp_rect.y = 0;
		}
		
		if (stamp_timer >= 8) {
			imagen = cpstamp_rect.y;
			
			SDL_BlitSurface (stamp_images[IMG_STAMP_PANEL], NULL, screen, &cpstamp_rect);
			cpstamp_rect.y = 0; cpstamp_rect.h = stamp_images[IMG_STAMP_PANEL]->h;
			
			/* Dibujar el texto de "Estampa ganada" */
			rect.x = 492; rect.y = imagen + 22;
			rect.w = stamp_earned[0]->w; rect.h = stamp_earned[0]->h;
			SDL_BlitSurface (stamp_earned[0], NULL, screen, &rect);
			
			rect.x = 490; rect.y = imagen + 20;
			rect.w = stamp_earned[1]->w; rect.h = stamp_earned[1]->h;
			SDL_BlitSurface (stamp_earned[1], NULL, screen, &rect);
			
			/* Dibujar subtitulo */
			rect.x = 492; rect.y = imagen + 42;
			rect.w = stamp_earned[0]->w; rect.h = stamp_earned[0]->h;
			SDL_BlitSurface (subtext[0], NULL, screen, &rect);
			
			rect.x = 490; rect.y = imagen + 40;
			rect.w = stamp_earned[1]->w; rect.h = stamp_earned[1]->h;
			SDL_BlitSurface (subtext[1], NULL, screen, &rect);
			
			rect.y = imagen + 6;
			if (local->categoria == STAMP_TYPE_GAME) {
				imagen = IMG_STAMP_GAME_EASY + local->dificultad;
			}
			
			rect.x = 410 + (73 - stamp_images[imagen]->w) / 2;
			rect.w = stamp_images[imagen]->w;
			rect.h = stamp_images[imagen]->h;
			
			SDL_BlitSurface (stamp_images[imagen], NULL, screen, &rect);
		}
		
		stamp_timer++;
	} else if (stamp_timer >= 56) {
		stamp_timer = 0;
		stamp_queue_start = (stamp_queue_start + 1) % 10;
		
		SDL_FreeSurface (subtext[0]);
		SDL_FreeSurface (subtext[1]);
	}
}

void CPStamp_Restore (SDL_Surface *screen) {
	SDL_Rect rect;
	
	if (stamp_timer > 8 && stamp_timer <= 56) {
		rect.x = 392;
		rect.y = 0;
		rect.h = stamp_images[IMG_STAMP_PANEL]->h;
		rect.w = stamp_images[IMG_STAMP_PANEL]->w;
		
		SDL_BlitSurface (save_screen, NULL, screen, &rect);
	}
}

CPStampCategory *CPStamp_Open (int tipo, char *nombre, char *clave) {
	char buf[4096];
	int fd;
	uint32_t temp;
	int g, n_stampas;
	CPStampCategory *abierta;
	CPStamp *s, *last;
	
	if (userdatalib_path == NULL || userdatalib_path[0] == 0) return NULL;
	
	sprintf (buf, "%s/.cpstamps/", userdatalib_path);
	
	if (!folder_exists (buf)) {
		if (!folder_create (buf)) {
			return NULL;
		}
	}
	
	sprintf (buf, "%s/.cpstamps/%s", userdatalib_path, clave);
	
	fd = open (buf, O_RDWR | O_CREAT, 0644);
	
	if (fd < 0) {
		if (errno == ENOENT) {
			return NULL;
		}
		perror (_("Failed to open Stamps File"));
	}
	
	abierta = (CPStampCategory *) malloc (sizeof (CPStampCategory));
	
	if (abierta == NULL) {
		return NULL;
	}
	
	abierta->nombre = nombre;
	abierta->tipo = tipo;
	abierta->fd = fd;
	abierta->lista = NULL;
	
	if (read (fd, &temp, sizeof (uint32_t)) == 0) {
		/* El archivo no existe, así que se ignora y se crea la estructura */
		return abierta;
	}
	
	if (read (fd, &temp, sizeof (uint32_t)) == 0) {
		/* Se llegó al fin de archivo, ignorar y retornar la estructura */
		return abierta;
	}
	
	n_stampas = temp;
	
	for (g = 0; g < n_stampas; g++) {
		s = (CPStamp *) malloc (sizeof (CPStamp));
	
		if (s == NULL) {
			free (abierta);
			return NULL;
		}
		read (fd, &temp, sizeof (uint32_t));
		s->id = temp;
		
		read (fd, &temp, sizeof (uint32_t));
		read (fd, buf, temp * sizeof (char));
		
		s->titulo = strdup (buf);
		read (fd, &temp, sizeof (uint32_t));
		s->categoria = temp;
		
		read (fd, &temp, sizeof (uint32_t));
		s->dificultad = temp;
		
		read (fd, &temp, sizeof (uint32_t));
		s->ganada = (temp != FALSE) ? TRUE : FALSE;
		
		if (g == 0) {
			abierta->lista = s;
		} else {
			last->sig = s;
		}
		
		last = s;
	}
	
	last->sig = NULL;
	
	return abierta;
}

void CPStamp_Register (CPStampCategory *cat, int id, char *titulo, char *descripcion, char *imagen, int categoria, int dificultad) {
	CPStamp *s, **t;
	if (cat == NULL) return;
	
	s = (CPStamp *) malloc (sizeof (CPStamp));
	if (s == NULL) return;
	s->sig = NULL;
	
	t = &(cat->lista);
	while (*t != NULL) t = &((*t)->sig);
	
	*t = s;
	
	s->id = id;
	s->titulo = strdup (titulo);
	s->categoria = categoria;
	s->dificultad = dificultad;
	s->ganada = FALSE;
}

int CPStamp_IsRegistered (CPStampCategory *cat, int id) {
	CPStamp *local;
	
	if (cat == NULL) return;
	local = cat->lista;
	
	while (local != NULL) {
		if (local->id == id) {
			return TRUE;
		}
		
		local = local->sig;
	}
	
	return FALSE;
}

void CPStamp_Close (CPStampCategory *cat) {
	uint32_t temp;
	int g;
	CPStamp *s, *last;
	
	if (cat == NULL) return;
	
	/* Rebobinar la posición del archivo */
	lseek (cat->fd, 0, SEEK_SET);
	
	/* Contar las estampas */
	s = cat->lista;
	
	g = 0;
	while (s != NULL) {
		g++;
		s = s->sig;
	}
	
	temp = 0; /* Versión del archivo */
	write (cat->fd, &temp, sizeof (uint32_t));
	
	temp = g; /* Número de estampas */
	write (cat->fd, &temp, sizeof (uint32_t));
	
	s = cat->lista;
	while (s != NULL) {
		temp = s->id;
		write (cat->fd, &temp, sizeof (uint32_t));
		
		temp = strlen (s->titulo) + 1;
		write (cat->fd, &temp, sizeof (uint32_t));
		
		write (cat->fd, s->titulo, temp * sizeof (char));
		
		free (s->titulo);
		
		temp = s->categoria;
		write (cat->fd, &temp, sizeof (uint32_t));
		
		temp = s->dificultad;
		write (cat->fd, &temp, sizeof (uint32_t));
		
		temp = s->ganada;
		write (cat->fd, &temp, sizeof (uint32_t));
		last = s;
		s = s->sig;
		
		free (last);
	}
	
	close (cat->fd);
	free (cat);
}

void CPStamp_ClearStamps (CPStampCategory *cat) {
	CPStamp *s;
	
	if (cat == NULL) return;
	s = cat->lista;
	
	while (s != NULL) {
		s->ganada = FALSE;
		s = s->sig;
	}
}

