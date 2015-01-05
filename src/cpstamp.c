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
#ifndef __MINGW32__
#include <pwd.h>
#endif

#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include "cpstamp.h"

const char *home_directory = NULL;

struct Stamp {
	int id;
	char *titulo;
	char *descripcion;
	char *imagen;
	
	int categoria;
	int dificultad;
	
	int ganada;
	
	Stamp *sig;
};

struct Categoria {
	char *nombre;
	int tipo;
	
	Stamp *lista;
	
	int fd;
};

/* Prototipos privados */
void get_home (void);

enum {
	IMG_STAMP_PANEL,
	
	IMG_STAMP_GAME_EASY,
	IMG_STAMP_GAME_NORMAL,
	IMG_STAMP_GAME_HARD,
	IMG_STAMP_GAME_EXTREME,
	
	NUM_IMGS
};

#ifdef __MINGW32__
#	define GAMEDATA_DIR "./"
#endif

const char * stamp_images_names [NUM_IMGS] = {
	GAMEDATA_DIR "images/panel_stamp.png",
	
	GAMEDATA_DIR "images/game_easy.png",
	GAMEDATA_DIR "images/game_normal.png",
	GAMEDATA_DIR "images/game_hard.png",
	GAMEDATA_DIR "images/game_extreme.png"
};

SDL_Surface *stamp_images [NUM_IMGS];
SDL_Surface *save_screen;
SDL_Surface *stamp_earned[2];

TTF_Font *font;

Mix_Chunk *stamp_sound_earn;

int stamp_queue[10];
int stamp_queue_start, stamp_queue_end;
int stamp_timer;

SDL_Rect stamp_rect;
int activar_estampa;

int iniciarCPStamp (void) {
	int g, h;
	SDL_Color blanco, negro;
	
	/* Conseguir el directorio del usuario actual */
	get_home ();
	
	for (g = 0; g < NUM_IMGS; g++) {
		stamp_images[g] = IMG_Load (stamp_images_names [g]);
		
		if (stamp_images[g] == NULL) {
			for (h = 0; h < g; h++) {
				SDL_FreeSurface (stamp_images[h]);
			}
			
			return -1;
		}
	}
	
	save_screen = SDL_AllocSurface (SDL_SWSURFACE, stamp_images[IMG_STAMP_PANEL]->w, stamp_images[IMG_STAMP_PANEL]->h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	
	stamp_sound_earn = Mix_LoadWAV (GAMEDATA_DIR "sounds/earn.wav");
	
	activar_estampa = stamp_timer = stamp_queue_start = stamp_queue_end = 0;
	
	if (!TTF_WasInit ()) {
		TTF_Init ();
	}
	
	if (TTF_WasInit ()) {
		font = TTF_OpenFont (GAMEDATA_DIR "burbanksb.ttf", 11);
		
		if (font != NULL) {
			blanco.r = blanco.g = blanco.b = 255;
			negro.r = negro.g = negro.b = 0;
			
			stamp_earned[0] = TTF_RenderUTF8_Blended (font, "Stamp Earned!", negro);
			stamp_earned[1] = TTF_RenderUTF8_Blended (font, "Stamp Earned!", blanco);
		} else {
			stamp_earned [0] = stamp_earned [1] = NULL;
		}
	} else {
		font = NULL;
		stamp_earned [0] = stamp_earned [1] = NULL;
	}
}

void earn_stamp (Categoria *cat, int id) {
	Stamp *s;
	
	s = cat->lista;
	
	while (s != NULL) {
		if (s->id == id) {
			if (!s->ganada) {
				activar_estampa = 1;
				s->ganada = TRUE;
				stamp_queue [stamp_queue_end] = id;
				stamp_queue_end = (stamp_queue_end + 1) % 10;
			}
			break;
		}
		
		s = s->sig;
	}
}

void dibujar_estampa (SDL_Surface *screen, Categoria *cat, int save) {
	SDL_Rect rect;
	int imagen;
	static Stamp *local;
	static SDL_Surface *subtext[2];
	
	if (cat == NULL) return;
	if (stamp_queue_start == stamp_queue_end) {
		activar_estampa = 0;
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
		stamp_rect.x = 392; stamp_rect.y = 0;
		stamp_rect.w = stamp_images[IMG_STAMP_PANEL]->w;
		stamp_rect.h = stamp_images[IMG_STAMP_PANEL]->h;
		
		SDL_BlitSurface (screen, &stamp_rect, save_screen, NULL);
	}
	
	if (stamp_timer < 56) {
		stamp_rect.h = stamp_images[IMG_STAMP_PANEL]->h;
		stamp_rect.w = stamp_images[IMG_STAMP_PANEL]->w;
		stamp_rect.x = 392;
		switch (stamp_timer) {
			case 8:
				stamp_rect.y = 20 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 9:
				stamp_rect.y = 40 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 10:
				stamp_rect.y = 53 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 11:
				Mix_PlayChannel (-1, stamp_sound_earn, 0);
				stamp_rect.y = 66 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 12:
				stamp_rect.y = 72 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 13:
				stamp_rect.y = 78 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 52:
				stamp_rect.y = 77 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 53:
				stamp_rect.y = 67 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 54:
				stamp_rect.y = 51 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 55:
				stamp_rect.y = 29 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
		}
		
		if (stamp_timer > 13 && stamp_timer < 52) {
			stamp_rect.y = 0;
		}
		
		if (stamp_timer >= 8) {
			imagen = stamp_rect.y;
			
			SDL_BlitSurface (stamp_images[IMG_STAMP_PANEL], NULL, screen, &stamp_rect);
			stamp_rect.y = 0; stamp_rect.h = stamp_images[IMG_STAMP_PANEL]->h;
			
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

void restaurar_dibujado (SDL_Surface *screen) {
	SDL_Rect rect;
	
	if (stamp_timer > 8 && stamp_timer <= 56) {
		rect.x = 392;
		rect.y = 0;
		rect.h = stamp_images[IMG_STAMP_PANEL]->h;
		rect.w = stamp_images[IMG_STAMP_PANEL]->w;
		
		SDL_BlitSurface (save_screen, NULL, screen, &rect);
	}
}

Categoria *abrir_cat (int tipo, char *nombre, char *clave) {
	char buf[4096];
	DIR *p;
	int fd;
	uint32_t temp;
	int g, n_stampas;
	Categoria *abierta;
	Stamp *s, *last;
	
	
	if (home_directory == NULL) return NULL;
	
	sprintf (buf, "%s/.cpstamps/", home_directory);
	
	p = opendir (buf);
	
	if (p == NULL) {
		/* Falló al intentar abrir el directorio,
		 * Si el error es no existe, intentar crear el directorio */
		if (errno == ENOENT) {
#ifdef __MINGW32__
			if (mkdir (buf) < 0) {
#else
			if (mkdir (buf, 0777) < 0) {
#endif
				perror ("Falló al crear el directorio de configuración de estampas");
			}
		} else {
			perror ("Falló al abrir el directorio de estampas");
		}
	} else {
		closedir (p);
	}
	
	sprintf (buf, "%s/.cpstamps/%s", home_directory, clave);
	
	fd = open (buf, O_RDWR | O_CREAT, 0644);
	
	if (fd < 0) {
		if (errno == ENOENT) {
			return NULL;
		}
		perror ("Falló al abrir el archivo de estampas");
	}
	
	abierta = (Categoria *) malloc (sizeof (Categoria));
	
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
		s = (Stamp *) malloc (sizeof (Stamp));
	
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

void registrar_estampa (Categoria *cat, int id, char *titulo, char *descripcion, char *imagen, int categoria, int dificultad) {
	Stamp *s, **t;
	if (cat == NULL) return;
	
	s = (Stamp *) malloc (sizeof (Stamp));
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

int esta_registrada (Categoria *cat, int id) {
	Stamp *local;
	
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

void cerrar_registro (Categoria *cat) {
	uint32_t temp;
	int g;
	Stamp *s, *last;
	
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

void get_home (void) {
#ifdef __MINGW32__
	home_directory = strdup ("./");
#else
	if (home_directory != NULL) return;
	struct passwd *entry;
	
	while ((entry = getpwent ()) != NULL) {
		if (entry->pw_uid == getuid ()) {
			home_directory = strdup (entry->pw_dir);
		}
	}
	
	endpwent ();
#endif
}
