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
#include <pwd.h>

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

SDL_Surface *pantalla;

enum {
	IMG_STAMP_PANEL,
	
	IMG_STAMP_GAME_EASY,
	IMG_STAMP_GAME_NORMAL,
	IMG_STAMP_GAME_HARD,
	IMG_STAMP_GAME_EXTREME,
	
	NUM_IMGS
};

const char * stamp_images_names [NUM_IMGS] = {
	GAMEDATA_DIR "images/panel_stamp.png",
	
	GAMEDATA_DIR "images/game_easy.png",
	GAMEDATA_DIR "images/game_normal.png",
	GAMEDATA_DIR "images/game_hard.png",
	GAMEDATA_DIR "images/game_extreme.png"
};

SDL_Surface *stamp_images [NUM_IMGS];
SDL_Surface *save_screen;

int stamp_queue[10];
int stamp_queue_start, stamp_queue_end;

int iniciarCPStamp (void) {
	int g, h;
	
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
	
	stamp_queue_start = stamp_queue_end = 0;
}

void earn_stamp (Categoria *c, int id) {
	Stamp *s;
	
	s = c->lista;
	
	while (s != NULL) {
		if (s->id == id) {
			if (!s->ganada) {
				s->ganada = TRUE;
				stamp_queue [stamp_queue_end] = id;
				stamp_queue_end = (stamp_queue_end + 1) % 10;
			}
			break;
		}
		
		s = s->sig;
	}
}

void dibujar_estampa (SDL_Surface *screen, int save) {
	SDL_Rect rect;
	static int timer = 0;
	
	if (stamp_queue_start == stamp_queue_end) return;
	
	if (save) {
		rect.x = 392; rect.y = 0;
		rect.w = stamp_images[IMG_STAMP_PANEL]->w;
		rect.h = stamp_images[IMG_STAMP_PANEL]->h;
		
		SDL_BlitSurface (screen, &rect, save_screen, NULL);
	}
	
	if (timer >= 8 && timer < 56) {
		rect.h = stamp_images[IMG_STAMP_PANEL]->h;
		rect.w = stamp_images[IMG_STAMP_PANEL]->w;
		rect.x = 392;
		switch (timer) {
			case 8:
				rect.y = 20 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 9:
				rect.y = 40 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 10:
				rect.y = 53 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 11:
				rect.y = 66 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 12:
				rect.y = 72 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 13:
				rect.y = 78 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 52:
				rect.y = 77 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 53:
				rect.y = 67 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 54:
				rect.y = 51 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 55:
				rect.y = 29 - stamp_images[IMG_STAMP_PANEL]->h;
				break;
		}
		if (timer > 13 && timer < 52) {
			rect.y = 0;
		}
		
		SDL_BlitSurface (stamp_images[IMG_STAMP_PANEL], NULL, screen, &rect);
		timer++;
	} else {
		timer = 0;
		stamp_queue_start = (stamp_queue_start + 1) % 10;
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
	
	printf ("Abriendo categoria: %s\n", clave);
	
	if (home_directory == NULL) return NULL;
	
	printf ("Home final: %s\n", home_directory);
	sprintf (buf, "%s/.cpstamps/", home_directory);
	
	p = opendir (buf);
	
	if (p == NULL) {
		/* Falló al intentar abrir el directorio,
		 * Si el error es no existe, intentar crear el directorio */
		if (errno == ENOENT) {
			if (mkdir (buf, 0777) < 0) {
				perror ("Falló al crear el directorio de configuración de estampas");
			}
		} else {
			perror ("Falló al abrir el directorio de estampas");
		}
	} else {
		closedir (p);
	}
	
	sprintf (buf, "%s/.cpstamps/%s", home_directory, clave);
	
	fd = open (buf, O_RDONLY);
	
	if (fd < 0) {
		if (errno == ENOENT) {
			return NULL;
		}
		perror ("Falló al abrir el archivo de estampas");
	}
	
	read (fd, &temp, sizeof (uint32_t));
	printf ("Abriendo el archivo de estampas %s, versión %i\n", clave, temp);
	
	abierta = (Categoria *) malloc (sizeof (Categoria));
	
	if (abierta == NULL) {
		return NULL;
	}
	
	abierta->nombre = nombre;
	abierta->tipo = tipo;
	abierta->fd = fd;
	
	read (fd, &temp, sizeof (uint32_t));
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
	
	last->sig == NULL;
	
	return abierta;
}

Categoria *crear_cat (int tipo, char *nombre, char *clave) {
	char buf[4096];
	DIR *p;
	int fd;
	uint32_t temp;
	int g, n_stampas;
	Categoria *abierta;
	Stamp *s, *last;
	
	printf ("Abriendo categoria: %s\n", clave);
	/* Conseguir el directorio del usuario actual */
	get_home ();
	
	if (home_directory == NULL) return NULL;
	
	printf ("Home final: %s\n", home_directory);
	sprintf (buf, "%s/.cpstamps/", home_directory);
	
	p = opendir (buf);
	
	if (p == NULL) {
		/* Falló al intentar abrir el directorio,
		 * Si el error es no existe, intentar crear el directorio */
		if (errno == ENOENT) {
			if (mkdir (buf, 0777) < 0) {
				perror ("Falló al crear el directorio de configuración de estampas");
			}
		} else {
			perror ("Falló al abrir el directorio de estampas");
		}
	} else {
		closedir (p);
	}
	
	sprintf (buf, "%s/.cpstamps/%s", home_directory, clave);
	
	fd = open (buf, O_WRONLY | O_CREAT, 0644);
	
	abierta = (Categoria *) malloc (sizeof (Categoria));
	
	if (abierta == NULL) {
		return NULL;
	}
	
	abierta->nombre = nombre;
	abierta->tipo = tipo;
	abierta->fd = fd;
	abierta->lista = NULL;
	
	return abierta;
}

void registrar_estampa (Categoria *cat, int id, char *titulo, int categoria, int dificultad) {
	Stamp *s, **t;
	if (cat == NULL) return;
	
	s = (Stamp *) malloc (sizeof (Stamp));
	if (s == NULL) return;
	s->sig = NULL;
	
	t = &(cat->lista);
	while (*t != NULL) t = &((*t)->sig);
	
	*t = s;
	
	s->id = id;
	s->titulo = titulo;
	s->categoria = categoria;
	s->dificultad = dificultad;
	s->ganada = FALSE;
}

void cerrar_registro (Categoria *cat) {
	uint32_t temp;
	int g;
	Stamp *s, *last;
	
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
	if (home_directory != NULL) return;
	printf ("Entrando a get_home\n");
	struct passwd *entry;
	
	while ((entry = getpwent ()) != NULL) {
		if (entry->pw_uid == getuid ()) {
			home_directory = strdup (entry->pw_dir);
		}
	}
	
	endpwent ();
}
