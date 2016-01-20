/*
 * path.h
 * This file is part of Paddle Puffle
 *
 * Copyright (C) 2015 - Félix Arreola Rodríguez
 *
 * Paddle Puffle is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Paddle Puffle is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Paddle Puffle. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PATH_H__
#define __PATH_H__

#define CPSTAMP_BUNDLE "org.cp.stamp"

void cpstamp_init_paths (const char *argv_0, char **systemdata_path, char **l10n_path, char **userdata_path);

int cpstamp_split_path (const char *path, char * dir_part, char * filename_part);

#endif /* __PATH_H__ */

