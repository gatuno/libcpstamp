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
#define _(string) dgettext (PACKAGE, string)

#include "cpstamp.h"
#include "path.h"

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

/* Lista de imágenes para cargar */
enum {
	IMG_STAMP_PANEL,
	
	IMG_STAMP_GAME_EASY,
	IMG_STAMP_GAME_NORMAL,
	IMG_STAMP_GAME_HARD,
	IMG_STAMP_GAME_EXTREME,
	
	NUM_IMGS
};

typedef struct _CPStamp {
	int id;
	char *titulo;
	char *descripcion;
	char *imagen;
	
	int categoria;
	int dificultad;
	
	int ganada;
	
	struct _CPStamp *sig;
} CPStamp;

struct _CPStampCategory {
	char *nombre;
	int tipo;
	
	CPStamp *lista;
	
	int fd;
};

struct _CPStampHandle {
	/* Paquete de imágenes */
	SDL_Surface *stamp_images [NUM_IMGS];
	SDL_Surface *save_screen;
	SDL_Surface *earned_text[2];
	
	/* Para renderizar los nombres de las estampas */
	TTF_Font *font;
	
	/* Sonido */
	int use_sound;
	Mix_Chunk *stamp_sound_earn;
	
	/* Lista privada de estampas que se deben dibujar */
	CPStamp *stamp_queue[10];
	int stamp_queue_start, stamp_queue_end;
	int stamp_timer;
	
	/* La carpeta del usuario */
	char *userdata_path;
	
	/* Para las aplicaciones */
	SDL_Rect update_rect;
	int activate;
};

/* Nombres de los archivos */
static const char * cpstamp_images_names [NUM_IMGS] = {
	"images/panel_stamp.png",
	
	"images/game_easy.png",
	"images/game_normal.png",
	"images/game_hard.png",
	"images/game_extreme.png"
};

/* Globales de la librería */
static CPStampHandle *cpstamp_handle = NULL;

/* Funciones locales auxiliares */
static int cpstamp_folder_exists (const char *fname) {
	struct stat s;
	return (stat(fname, &s) == 0 && S_ISDIR(s.st_mode));
}

static int cpstamp_file_exists (const char *fname) {
	struct stat s;
	return (stat(fname, &s) == 0 && S_ISREG(s.st_mode));
}

static int cpstamp_folder_create (const char *fname) {
	char *parent_folder;
	char *sub_folder;
	int ok = TRUE;
	
	if (cpstamp_folder_exists (fname)) return TRUE;
	
	parent_folder = strdup (fname);
	sub_folder = strdup (fname);
	
	if (cpstamp_split_path (fname, parent_folder, sub_folder)) {
		if (!cpstamp_folder_exists (parent_folder)) {
			ok = cpstamp_folder_create (parent_folder);
		}
	}
	
	if (ok) {
		#ifdef __MINGW32__
		ok = mkdir(fname) == 0;
		#else
		ok = mkdir(fname, 0775) == 0;
		#endif
	}
	
	free (parent_folder);
	free (sub_folder);
	return ok;
}

/* Funciones públicas */
CPStampHandle *CPStamp_Init (int argc, char **argv) {
	CPStampHandle *l_handle;
	char *systemdata_path, *l10n_path;
	int g, h;
	SDL_Color blanco, negro;
	char buffer_file[8192];
	
	/* Si ya nos inicializamos, regresar el handle */
	if (cpstamp_handle != NULL) return cpstamp_handle;
	
	l_handle = (CPStampHandle *) malloc (sizeof (CPStampHandle));
	
	if (l_handle == NULL) {
		/* Oops, sin memoria */
		return NULL;
	}
	
	l_handle->userdata_path = NULL;
	/* Conseguir las urls del sistema */
	cpstamp_init_paths (argv[0], &systemdata_path, &l10n_path, &l_handle->userdata_path);
	
	/* Inicializar nuestro dominio de i18n */
	bindtextdomain (PACKAGE, l10n_path);
	
	free (l10n_path);
	
	for (g = 0; g < NUM_IMGS; g++) {
		sprintf (buffer_file, "%s%s", systemdata_path, cpstamp_images_names [g]);
		l_handle->stamp_images[g] = IMG_Load (buffer_file);
		
		/* Si falla la carga de alguna de las imágenes, eliminar todo */ 
		if (l_handle->stamp_images[g] == NULL) {
			for (h = 0; h < g; h++) {
				SDL_FreeSurface (l_handle->stamp_images[h]);
				l_handle->stamp_images[h] = NULL;
			}
			
			free (l_handle);
			return NULL;
		}
	}
	
	l_handle->save_screen = SDL_AllocSurface (SDL_SWSURFACE, l_handle->stamp_images[IMG_STAMP_PANEL]->w, l_handle->stamp_images[IMG_STAMP_PANEL]->h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	
	sprintf (buffer_file, "%ssounds/earn.wav", systemdata_path);
	l_handle->stamp_sound_earn = Mix_LoadWAV (buffer_file);
	
	if (l_handle->stamp_sound_earn == NULL) {
		/* Desactivar el audio, no pude cargar un sonido */
		l_handle->use_sound = 0;
	}
	
	l_handle->activate = l_handle->stamp_timer = l_handle->stamp_queue_start = l_handle->stamp_queue_end = 0;
	
	if (!TTF_WasInit ()) {
		TTF_Init ();
	}
	
	if (TTF_WasInit ()) {
		sprintf (buffer_file, "%sburbanksb.ttf", systemdata_path);
		l_handle->font = TTF_OpenFont (buffer_file, 11);
		
		if (l_handle->font != NULL) {
			blanco.r = blanco.g = blanco.b = 255;
			negro.r = negro.g = negro.b = 0;
			
			l_handle->earned_text[0] = TTF_RenderUTF8_Blended (l_handle->font, _("Stamp Earned!"), negro);
			l_handle->earned_text[1] = TTF_RenderUTF8_Blended (l_handle->font, _("Stamp Earned!"), blanco);
		} else {
			l_handle->earned_text[0] = l_handle->earned_text[1] = NULL;
		}
	} else {
		l_handle->font = NULL;
		l_handle->earned_text[0] = l_handle->earned_text[1] = NULL;
	}
	
	cpstamp_handle = l_handle;
	
	return l_handle;
}

void CPStamp_Earn (CPStampHandle *handle, CPStampCategory *cat, int id) {
	CPStamp *s;
	
	if (handle == NULL || cat == NULL) return;
	s = cat->lista;
	
	while (s != NULL) {
		if (s->id == id) {
			if (!s->ganada) {
				handle->activate = 1;
				s->ganada = TRUE;
				handle->stamp_queue [handle->stamp_queue_end] = s;
				handle->stamp_queue_end = (handle->stamp_queue_end + 1) % 10;
			}
			break;
		}
		
		s = s->sig;
	}
}

void CPStamp_Draw (CPStampHandle *handle, SDL_Surface *screen, int save) {
	SDL_Rect rect;
	int imagen;
	static SDL_Surface *subtext[2];
	SDL_Color blanco, negro;
	CPStamp *stamp;
	
	if (handle == NULL) return;
	if (handle->stamp_queue_start == handle->stamp_queue_end) {
		handle->activate = 0;
		return;
	}
	
	/* Encontrar la estampa a mostrar */
	stamp = handle->stamp_queue[handle->stamp_queue_start];
	
	if (handle->stamp_timer == 0) {
		if (handle->font != NULL) {
			blanco.r = blanco.g = blanco.b = 255;
			negro.r = negro.g = negro.b = 0;
			
			subtext[0] = TTF_RenderUTF8_Blended (handle->font, stamp->titulo, negro);
			subtext[1] = TTF_RenderUTF8_Blended (handle->font, stamp->titulo, blanco);
		}
	}
	
	if (handle->stamp_timer >= 8 && save) {
		handle->update_rect.x = 392; handle->update_rect.y = 0;
		handle->update_rect.w = handle->stamp_images[IMG_STAMP_PANEL]->w;
		handle->update_rect.h = handle->stamp_images[IMG_STAMP_PANEL]->h;
		
		SDL_BlitSurface (screen, &handle->update_rect, handle->save_screen, NULL);
	}
	
	if (handle->stamp_timer < 56) {
		handle->update_rect.h = handle->stamp_images[IMG_STAMP_PANEL]->h;
		handle->update_rect.w = handle->stamp_images[IMG_STAMP_PANEL]->w;
		handle->update_rect.x = 392;
		switch (handle->stamp_timer) {
			case 8:
				handle->update_rect.y = 20 - handle->stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 9:
				handle->update_rect.y = 40 - handle->stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 10:
				handle->update_rect.y = 53 - handle->stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 11:
				if (handle->use_sound) Mix_PlayChannel (-1, handle->stamp_sound_earn, 0);
				handle->update_rect.y = 66 - handle->stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 12:
				handle->update_rect.y = 72 - handle->stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 13:
				handle->update_rect.y = 78 - handle->stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 52:
				handle->update_rect.y = 77 - handle->stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 53:
				handle->update_rect.y = 67 - handle->stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 54:
				handle->update_rect.y = 51 - handle->stamp_images[IMG_STAMP_PANEL]->h;
				break;
			case 55:
				handle->update_rect.y = 29 - handle->stamp_images[IMG_STAMP_PANEL]->h;
				break;
		}
		
		if (handle->stamp_timer > 13 && handle->stamp_timer < 52) {
			handle->update_rect.y = 0;
		}
		
		if (handle->stamp_timer >= 8) {
			imagen = handle->update_rect.y;
			
			SDL_BlitSurface (handle->stamp_images[IMG_STAMP_PANEL], NULL, screen, &handle->update_rect);
			handle->update_rect.y = 0; handle->update_rect.h = handle->stamp_images[IMG_STAMP_PANEL]->h;
			
			/* Dibujar los textos sólo si la tipografía funciona */
			if (handle->font != NULL) {
				/* Dibujar el texto de "Estampa ganada" */
				rect.x = 492; rect.y = imagen + 22;
				rect.w = handle->earned_text[0]->w; rect.h = handle->earned_text[0]->h;
				SDL_BlitSurface (handle->earned_text[0], NULL, screen, &rect);
			
				rect.x = 490; rect.y = imagen + 20;
				rect.w = handle->earned_text[1]->w; rect.h = handle->earned_text[1]->h;
				SDL_BlitSurface (handle->earned_text[1], NULL, screen, &rect);
			
				/* Dibujar subtitulo */
				rect.x = 492; rect.y = imagen + 42;
				rect.w = subtext[0]->w; rect.h = subtext[0]->h;
				SDL_BlitSurface (subtext[0], NULL, screen, &rect);
			
				rect.x = 490; rect.y = imagen + 40;
				rect.w = subtext[1]->w; rect.h = subtext[1]->h;
				SDL_BlitSurface (subtext[1], NULL, screen, &rect);
			}
			
			rect.y = imagen + 6;
			if (stamp->categoria == STAMP_TYPE_GAME) {
				imagen = IMG_STAMP_GAME_EASY + stamp->dificultad;
			} else {
				imagen = IMG_STAMP_GAME_EASY;
			}
			
			rect.x = 410 + (73 - handle->stamp_images[imagen]->w) / 2;
			rect.w = handle->stamp_images[imagen]->w;
			rect.h = handle->stamp_images[imagen]->h;
			
			SDL_BlitSurface (handle->stamp_images[imagen], NULL, screen, &rect);
		}
		
		handle->stamp_timer++;
	} else if (handle->stamp_timer >= 56) {
		handle->stamp_timer = 0;
		handle->stamp_queue_start = (handle->stamp_queue_start + 1) % 10;
		
		if (subtext[0] != NULL) {
			SDL_FreeSurface (subtext[0]);
			SDL_FreeSurface (subtext[1]);
		}
	}
}

void CPStamp_Restore (CPStampHandle *handle, SDL_Surface *screen) {
	SDL_Rect rect;
	
	if (handle->stamp_timer > 8 && handle->stamp_timer <= 56) {
		rect.x = 392;
		rect.y = 0;
		rect.h = handle->stamp_images[IMG_STAMP_PANEL]->h;
		rect.w = handle->stamp_images[IMG_STAMP_PANEL]->w;
		
		SDL_BlitSurface (handle->save_screen, NULL, screen, &rect);
	}
}

CPStampCategory *CPStamp_Open (CPStampHandle *handle, int tipo, char *nombre, char *clave) {
	char buf[4096];
	int fd;
	uint32_t temp;
	int g, n_stampas, res;
	CPStampCategory *abierta;
	CPStamp *s, *last;
	
	if (handle == NULL) return NULL;
	if (handle->userdata_path == NULL || handle->userdata_path[0] == 0) return NULL;
	
	sprintf (buf, "%s/.cpstamps/", handle->userdata_path);
	
	if (!cpstamp_folder_exists (buf)) {
		if (!cpstamp_folder_create (buf)) {
			return NULL;
		}
	}
	
	sprintf (buf, "%s/.cpstamps/%s", handle->userdata_path, clave);
	
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
	
	/* Leer el número de versión */
	if (read (fd, &temp, sizeof (uint32_t)) == 0) {
		/* El archivo no existe, así que se ignora y se crea la estructura */
		return abierta;
	}
	
	/* Si la versión no es 0, no abrir el archivo */
	if (temp != 0) {
		free (abierta);
		return NULL;
	}
	
	/* Leer la cantidad de estampas */
	if (read (fd, &temp, sizeof (uint32_t)) == 0) {
		/* Se llegó al fin de archivo, ignorar y retornar la estructura */
		return abierta;
	}
	
	n_stampas = temp;
	
	for (g = 0; g < n_stampas; g++) {
		s = (CPStamp *) malloc (sizeof (CPStamp));
		
		if (s == NULL) {
			return abierta;
		}
		s->sig = NULL;
		
		/* Leer el id de la estampa */
		res = read (fd, &temp, sizeof (uint32_t));
		if (res <= 0) {
			/* Error en la lectura del archivo, ignorar la estampa y salir */
			free (s);
			return abierta;
		}
		
		s->id = temp;
		
		temp = 0;
		res = read (fd, &temp, sizeof (uint32_t));
		if (res <= 0 || temp > 255) {
			/* Error en la lectura del archivo, ignorar la estampa y salir 
			 * o Cadena de texto demasiado larga */
			free (s);
			return abierta;
		}
		
		res = read (fd, buf, temp * sizeof (char));
		
		if (res < temp) {
			/* Hemos leído menos bytes que los esperados */
			free (s);
			return abierta;
		}
		
		buf[temp] = 0; /* Fin de cadena */
		
		s->titulo = strdup (buf);
		
		res = read (fd, &temp, sizeof (uint32_t));
		if (res < 0 || temp >= NUM_STAMP_TYPE) {
			/* Error de lectura 
			 * o dato inválido */
			free (s->titulo);
			free (s);
			return abierta;
		}
		
		s->categoria = temp;
		
		res = read (fd, &temp, sizeof (uint32_t));
		if (res < 0 || temp > STAMP_EXTREME) {
			/* Error de lectura 
			 * o dato inválido */
			free (s->titulo);
			free (s);
			return abierta;
		}
		
		s->dificultad = temp;
		
		res = read (fd, &temp, sizeof (uint32_t));
		if (res < 0) {
			/* Error de lectura */
			free (s->titulo);
			free (s);
			return abierta;
		}
		
		s->ganada = (temp != FALSE) ? TRUE : FALSE;
		
		if (g == 0) {
			abierta->lista = s;
		} else {
			last->sig = s;
		}
		
		last = s;
	}
	
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
	
	if (cat == NULL) return FALSE;
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

SDL_Rect CPStamp_GetUpdateRect (CPStampHandle *handle) {
	return handle->update_rect;
}

int CPStamp_IsActive (CPStampHandle *handle) {
	return handle->activate;
}

void CPStamp_WithSound (CPStampHandle *handle, int sound) {
	if (sound && handle->stamp_sound_earn != NULL) {
		/* Pidieron sonido, revisar si pude cargar el archivo de sonido */
		handle->use_sound = TRUE;
	} else {
		/* O no pidieron sonido, o de todas formas no pude cargar el archivo de sonido */
		handle->use_sound = FALSE;
	}
}
