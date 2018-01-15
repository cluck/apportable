/* apportable.c - Self-awareness, cross-platform
 *
 * Copyright (C) 2018 Claudio Luck
 *
 * This file is part of apportable.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either
 *
 *   - the GNU Lesser General Public License as published by the Free
 *     Software Foundation; either version 3 of the License, or (at
 *     your option) any later version.
 *
 * or
 *
 *   - the GNU General Public License as published by the Free
 *     Software Foundation; either version 2 of the License, or (at
 *     your option) any later version.
 *
 * or both in parallel, as here.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */


#ifndef APPORTABLE_H
#define APPORTABLE_H


typedef struct apportable_t
{
	int initialized;   /* keep this first, for initialization = {0} */
	int enabled;
	char * _iconv_wchar_t;
	void * (*_calloc) (size_t, size_t);
	void (*_free) (void *);

	char * (*_strndup) (struct apportable_t *, const char *, size_t);
	wchar_t * (*_wcsndup) (struct apportable_t *, const wchar_t *, size_t);

	char * (*wutf8) (struct apportable_t *, const wchar_t *);
	char * (*wutf8_free) (struct apportable_t *, wchar_t *);
	wchar_t * (*uwchar_t) (struct apportable_t *, const char *);
	wchar_t * (*uwchar_t_free) (struct apportable_t *, char *);
	char * (*ugetenv) (struct apportable_t *, char *);
	char * (*wugetenv) (struct apportable_t *, wchar_t *);

	char * (*whereis) (struct apportable_t *, const char *, const char *, int);
	char * (*progfile) (struct apportable_t *, const char *);
	char * (*pathexp) (struct apportable_t *, const char *, const char *);
}
	apportable_t, * apportable;


void apportable_new (apportable a);
void apportable_init (apportable a, int enabled);
apportable apportable_getstate (apportable a);

char * apportable_strndup (apportable a, const char * str, size_t size);
wchar_t * apportable_wcsndup (apportable a, const wchar_t * str, size_t syms);

char * apportable_wutf8 (apportable a, const wchar_t * s);
char * apportable_wutf8_free (apportable a, wchar_t * s);
wchar_t * apportable_uwchar_t (apportable a, const char * s);
wchar_t * apportable_uwchar_t_free (apportable a, char * s);
char * apportable_ugetenv (apportable a, char * var);
char * apportable_wugetenv (apportable a, wchar_t * wvar);

char * apportable_whereis (apportable a, const char * searchpath, const char * bin, int execonly);
char * apportable_progfile (apportable a, const char * library_name);
char * apportable_pathexp (apportable a, const char * template, const char * library_path);


#endif /*APPORTABLE_H*/
