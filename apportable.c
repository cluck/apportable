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


#if defined APPORTABLE

#if defined __APPLE__
# define APPORTABLE_SUPPORTED
# include <mach-o/dyld.h>
# include <mach-o/nlist.h>
# include <sys/syslimits.h>
# include <iconv.h>
# ifdef __LP64__
typedef struct mach_header_64 mach_header_t;
typedef struct segment_command_64 segment_command_t;
typedef struct nlist_64 nlist_t;
# else
typedef struct mach_header mach_header_t;
typedef struct segment_command segment_command_t;
typedef struct nlist nlist_t;
# endif

#elif defined _WIN32
# define APPORTABLE_SUPPORTED
# include <Windows.h>

#elif defined __BIONIC__  // Android
// #  define PORTABLE_H_ENABLED
# include <linker/linker.h>

#elif defined __GNUC__
# define _GNU_SOURCE
# ifdef __linux__
#  define APPORTABLE_SUPPORTED
#  include <linux/limits.h>
#  include <link.h>
#  include <dlfcn.h>
#  include <iconv.h>
# endif

#elif defined __UCLIBC__
// #  define PORTABLE_H_ENABLED
# include <ldso.h>

#endif /* platforms */

#if defined _WIN32
#define DIRSEP_S "\\"
#define DIRSEP_C '\\'
#define PATHSEP_S ";"
#define PATHSEP_C ';'
# if !defined(S_ISDIR) && defined(S_IFMT) && defined(S_IFDIR)
#  define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
# endif
#else
#define DIRSEP_S "/"
#define DIRSEP_C '/'
#define PATHSEP_S ":"
#define PATHSEP_C ':'
#endif


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>

#ifdef _WIN32
# include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>

// Debugging
#include <stdio.h>

#include "apportable.h"

#define mem_calloc(a, t, c) (a)->_calloc((c), sizeof(t))
#define mem_free(a, v) (a)->_free((v))
#define APPORTABLE_STATE(a) apportable_getstate((a))

static apportable_t apportable_global_state = {0, 0};


void apportable_init (
        apportable a,
        int enabled
)
{
    /* figure out endianness */
    int le;
#if !defined _WIN32
    volatile uint32_t le_i;

    le_i = 0x01234567;
    le = ((*((uint8_t*)(&le_i))) == 0x67);
#else /*_WIN32*/
    le = 1;
#endif
    if (sizeof(wchar_t) == 4)
        a->_iconv_wchar_t = le ? "UTF-32LE" : "UTF-32BE";
    else if (sizeof(wchar_t) == 2)
        a->_iconv_wchar_t = le ? "UTF-16LE" : "UTF-32BE";

    a->_calloc = calloc;
    a->_free = free;

    a->_strndup = &apportable_strndup;
    a->_wcsndup = &apportable_wcsndup;

    a->wutf8 = &apportable_wutf8;
    a->wutf8_free = &apportable_wutf8_free;
    a->uwchar_t = &apportable_uwchar_t;
    a->uwchar_t_free = &apportable_uwchar_t_free;

    a->ugetenv = &apportable_ugetenv;
    a->wugetenv = &apportable_wugetenv;

    a->progfile = &apportable_progfile;
    a->pathexp = &apportable_pathexp;
    a->whereis = &apportable_whereis;
    a->enabled = enabled;
    a->initialized = 1;



    return;
}


apportable apportable_getstate (apportable a)
{
    apportable self;
    int default_enabled = 0;
    if (a != NULL)
        self = a;
    else
    {
        self = &apportable_global_state;
        default_enabled = 1;
    }
    if (self->initialized)
        return self;
    apportable_init(self, default_enabled);
    return self;
}



/* UTF-8 -> wchar_t -> UTF-8 */
char * apportable_strndup(apportable a, const char * str, size_t syms)
{
    // WORKS

    apportable self;
    wchar_t * wstr, * buffer;
    char * ret;
    size_t buflen;

    self = APPORTABLE_STATE(a);
    if (str == NULL)
        return NULL;

    wstr = self->uwchar_t(self, str);
    buflen = wcslen(wstr);
    buffer = self->_wcsndup(self, wstr, syms);

    ret = self->wutf8(self, buffer);

    // // return ret;
    // char * x = malloc(1000);
    // sprintf(x, "%zu %zu L<%ls> <%s>", syms, buflen, buffer, ret);
    // return x;

    free(buffer);
    return ret;
}


wchar_t * apportable_wcsndup(apportable a, const wchar_t * str, size_t syms)
{
    apportable self;
    wchar_t * buffer;
    size_t buflen;
    size_t copysyms;

    self = APPORTABLE_STATE(a);
    if (str == NULL)
        return NULL;
    buflen = wcslen(str);
    if (syms == 0)
        syms = buflen;
    if (!(copysyms = syms))
        copysyms = (syms > buflen ? buflen : syms);
    if ((buffer = self->_calloc(sizeof(wchar_t), buflen + 1)))
        wcsncpy(buffer, str, copysyms);
    // sprintf(buffer, L"%d %d", strlen(buffer), strlen(str));
    return buffer;
}


#ifdef _WIN32

char * apportable_wutf8 (apportable a, const wchar_t * s)
{
    apportable self;
    char * b;
    size_t s_l, b_l;

    self = APPORTABLE_STATE(a);
    if (s == NULL)
        return NULL;
    s_l = wcslen(s);
    b_l = WideCharToMultiByte(CP_UTF8, 0, s, s_l, NULL, 0, NULL, NULL);
    b = self->_calloc(sizeof(wchar_t), b_l + 1);
    WideCharToMultiByte(CP_UTF8, 0, s, s_l, b, b_l, NULL, NULL);
    return b;
}

char * apportable_wutf8_free(apportable a, wchar_t * s)
{
    apportable self;
    char * ret;

    self = APPORTABLE_STATE(a);
    ret = self->wutf8(self, s);
    self->_free(s);
    return ret;
}


wchar_t * apportable_uwchar_t (apportable a, const char * s)
{
    apportable self;
    char * b;
    size_t s_l, b_l;

    self = APPORTABLE_STATE(a);
    if (s == NULL)
        return NULL;
    s_l = strlen(s) + 1;
    b_l = MultiByteToWideChar(CP_UTF8, 0, s, s_l, NULL, 0);
    b = self->_calloc(sizeof(wchar_t), b_l + 0);
    MultiByteToWideChar(CP_UTF8, 0, s, s_l, b, b_l);
    return b;
}

wchar_t * apportable_uwchar_t_free (apportable a, const char * s)
{
    apportable self;
    char * ret;

    self = APPORTABLE_STATE(a);
    ret = self->uwchar_t(self, s);
    self->_free(s);
    return ret;
}



char * apportable_ugetenv (apportable a, const char * var)
{
    apportable self;
    wchar_t * v;
    size_t var_l, v_l;

    self = APPORTABLE_STATE(a);
    var_l = strlen(var) + 1;
    v_l = MultiByteToWideChar(CP_UTF8, 0, var, var_l, NULL, 0);
    v = self->_calloc(sizeof(wchar_t), v_l + 0);
    MultiByteToWideChar(CP_UTF8, 0, var, var_l, v, v_l);
    return self->wutf8(self, _wgetenv(v));
}

char * apportable_wugetenv (apportable a, const wchar_t * var)
{
    apportable self;
    wchar_t * v;
    size_t var_l, v_l;

    self = APPORTABLE_STATE(a);
    var_l = wcslen(var) + 1;
    v_l = MultiByteToWideChar(CP_UTF8, 0, var, var_l, NULL, 0);
    v = self->_calloc(sizeof(wchar_t), v_l + 0);
    MultiByteToWideChar(CP_UTF8, 0, var, var_l, v, v_l);
    return self->wutf8(self, _wgetenv(v));
}






#else /*!_WIN32*/

char * apportable_wutf8 (apportable a, const wchar_t * s)
{
    apportable self;
    char * ret;
    size_t s_l, ret_l;
    /* iconv expects in-out params, so allocate copies */
    wchar_t * iconv_s;
    char * iconv_ret;
    size_t iconv_s_l, iconv_ret_l;
    iconv_t iconv_obj;

    self = APPORTABLE_STATE(a);
    s_l = sizeof(wchar_t) * ( wcslen(s) + 1 );
    iconv_obj = iconv_open("UTF-8", self->_iconv_wchar_t);
    if (iconv_obj == (iconv_t) -1) {
        return NULL;
    }
    ret_l = sizeof(char) * ( s_l + 1 );
    ret = self->_calloc(1, ret_l);

    iconv_s = (wchar_t *) s, iconv_ret = ret;
    iconv_s_l = s_l, iconv_ret_l = ret_l;
    iconv(iconv_obj, (char **)&iconv_s, &iconv_s_l, &iconv_ret, &iconv_ret_l);
    iconv_close(iconv_obj);

    // char * b = malloc(1000);
    // sprintf(b, "%zu '%ls' '%s'", ret_l, s, ret);
    // printf("%s\n", b);
    // return b;

    return ret;
}

char * apportable_wutf8_free (apportable a, wchar_t * s)
{
    apportable self;
    char * ret;

    self = APPORTABLE_STATE(a);
    ret = self->wutf8(self, s);
    self->_free(s);
    return ret;
}



wchar_t * apportable_uwchar_t (apportable a, const char * s)
{
    apportable self;
    wchar_t * ret;
    wchar_t * buffer;
    size_t s_l, buffer_l;
    /* iconv expects in-out params, so allocate copies */
    char * iconv_s;
    wchar_t * iconv_buffer;
    size_t iconv_s_l, iconv_buffer_l;
    iconv_t iconv_obj;

    self = APPORTABLE_STATE(a);
    s_l = sizeof(char) * ( strlen(s) + 1 );
    iconv_obj = iconv_open(self->_iconv_wchar_t, "UTF-8");
    if (iconv_obj == (iconv_t) -1) {
        return NULL;
    }
    buffer_l = sizeof(wchar_t) * ( s_l + 1 );
    buffer = self->_calloc(1, buffer_l);

    iconv_s = (char *) s, iconv_buffer = buffer;
    iconv_s_l = s_l, iconv_buffer_l = buffer_l;
    iconv(iconv_obj, (char **)&iconv_s, &iconv_s_l, (char **) &iconv_buffer, &iconv_buffer_l);
    
    // char * b2 = malloc(1000);
    // sprintf(b2, "%d->%d %d->%d <%ls> <%s>", s_l, iconv_s_l, buffer_l, iconv_buffer_l, s, buffer);
    // return b2;
    
    iconv_close(iconv_obj);
    ret = self->_wcsndup(self, buffer, 0);
    self->_free(buffer);
    return ret;
}

wchar_t * apportable_uwchar_t_free (apportable a, char * s)
{
    apportable self;
    wchar_t * ret;

    self = APPORTABLE_STATE(a);
    ret = self->uwchar_t(self, s);
    self->_free(s);
    return ret;
}




char * apportable_ugetenv (apportable a, char * var)
{
    apportable self;
    char * ret;

    self = APPORTABLE_STATE(a);
    if (!(ret = self->_strndup(self, getenv(var), 0)))
        ret = self->_strndup(self, "", 0);
    return ret;
}

char * apportable_wugetenv (apportable a, wchar_t * wvar)
{
    apportable self;
    char * var, * ret;

    self = APPORTABLE_STATE(a);
    var = self->wutf8(self, wvar);
    if (!(ret = self->ugetenv(self, var)))
        ret = self->_strndup(self, "", 0);
    self->_free(var);
    return ret;
}

#endif



#ifdef _WIN32

/* for UTF-16 "wide" character */
wchar_t * apportable_wprogfile (apportable a, const wchar_t * library_name) {
    apportable self;
	wchar_t * ret;
	HMODULE handle;
    wchar_t * wfile;
    wchar_t * file;

    self = APPORTABLE_STATE(a);
    if (!self->enabled)
        return self->_wcsndup(self, library_name, 0);

    ret = NULL;
    handle = NULL;
    if (!GetModuleHandleExW(0, library_name, &handle)) {
        if (handle)
            FreeLibrary(handle);
        return NULL;
    }
    wfile = self->_calloc(sizeof(WCHAR), MAX_PATH);
    file = self->_calloc(sizeof(char), MAX_PATH);
    if (GetModuleFileNameW(handle, wfile, MAX_PATH))
	    ret = file;
	self->_free(wfile);
	if (!ret)
		self->_free(file);
	FreeLibrary(handle);
	return ret;
}

/* for utf-8, will call into native UTF-16 */
char * apportable_progfile (apportable a, const char * library_name) {
    apportable self;
    self = APPORTABLE_STATE(a);
    if (!self->enabled)
        return self->_strndup(a, (const char *)library_name, 0);

	char * ret = NULL;
    PWSTR wlibnam;
    size_t ln_l, wlibnam_l;
	if (library_name == NULL)
		return apportable_wprogfile (self, NULL);
    ln_l = strlen(library_name) + 1;
    wlibnam_l = MultiByteToWideChar (CP_UTF8, 0, library_name, ln_l, NULL, 0);
    wlibnam = self->_calloc (sizeof(char), wlibnam_l + 0);
	if (!MultiByteToWideChar (CP_UTF8, 0, library_name, ln_l, wlibnam, wlibnam_l))
    {
		self->_free(wlibnam);
		return NULL;			
	}
	ret = apportable_wprogfile (a, wlibnam);
	self->_free(wlibnam);
	return ret;
}

#elif defined __APPLE__

char * apportable_progfile (apportable a, const char * library_name) {
    apportable self;
    self = APPORTABLE_STATE(a);
    if (!self->enabled)
        return self->_strndup(self, library_name, 0);

    const char* library_base_name = library_name ? strrchr(library_name, DIRSEP_C) : NULL;
    if (!library_base_name)
        library_base_name = library_name;
    else
        library_base_name += 1;   // skip found '/'
    char* library_file = NULL;

    const char * image_name = NULL;
    const char * library_name_sep = NULL;
    unsigned long image_count = _dyld_image_count();
    errno = 0;
    for (unsigned long i = 0; i < image_count; i++) {
        image_name = (const char *)_dyld_get_image_name(i);  // dyld owns string
        if (!image_name || !image_name[0])
            continue;
        library_name_sep = strrchr(image_name, '/');
        if (!library_name_sep)
            library_name_sep = image_name;
       	else
       		library_name_sep += 1;   // skip found '/'
        if (!library_name || !strcmp(library_name_sep, library_base_name)) {
            library_file = self->_strndup(a, image_name, PATH_MAX);
            break;
        }
    }
    return library_file;
}

#elif defined __GNUC__

extern char *program_invocation_name;

wchar_t * apportable_progfile (apportable a, const wchar_t * library_name) {
    apportable self;
    self = APPORTABLE_STATE(a);
    if (!self->enabled)
        return self->_strndup(a, library_name, 0);

	const char* library_base_name = library_name ? strrchr(library_name, DIRSEP_C) : NULL;
    if (!library_base_name)
        library_base_name = library_name;
    else
        library_base_name += 1;   // skip found '/'
    char * library_file = NULL;
    char * image_name;
    char * image_name_real;
    char * library_name_sep;
    FILE * cmdline;
    
    void *handle = dlopen(NULL, RTLD_LAZY);
    if (!handle)
        return NULL;
    const struct link_map * link_map = 0;
    int ret = dlinfo(handle, RTLD_DI_LINKMAP, &link_map);
    if (ret || !link_map) {
        dlclose(handle);
        return NULL;
    }
    image_name_real = realpath(program_invocation_name, NULL);
    /* sometimes program_invocation_name gets wiped for reasons of beauty... */
    if (!image_name_real || !image_name_real[0]) {
        if ((cmdline = fopen("/proc/self/cmdline", "rb"))) {
            char *arg = 0;
            size_t size = 0;
            while (getdelim(&arg, &size, 0, cmdline) != -1) {
                image_name_real = arg;
                break;
            }
            fclose(cmdline);
        }        
    }
    while (link_map->l_prev)
        link_map = link_map->l_prev;
    for(int i = 0; link_map->l_next != NULL; link_map = link_map->l_next, i++) {
        if (link_map->l_name && link_map->l_name[0])
            image_name = link_map->l_name;   /* Absolute pathname according to dlinfo(3) */
        else {
            if (i == 0 && image_name_real != NULL)
                image_name = image_name_real;
        }
        if (!image_name || !image_name[0])
            continue;
        library_name_sep = strrchr(image_name, DIRSEP_C);
        if (!library_name_sep)
            library_name_sep = image_name;
        else
            library_name_sep += 1;   // skip found '/'
        if (!library_name || !strcmp(library_name_sep, library_base_name)) {
            library_file = self->_strndup(a, image_name, PATH_MAX);
            break;
        }
    }
    if (image_name_real)
        free(image_name_real);
    return library_file;
}


// #else

// char * apportable_progfile (apportable a, const wchar_t * library_name) {
// 	return NULL;
// }

// char * apportable_pathexp(apportable a, const wchar_t * template, const wchar_t * library_path) {
// 	return self->_strndup(a, template, strlen(template));
// }

#endif /*_WIN32, __APPLE__, ...*/


char * apportable_whereis(apportable a, const char * searchpath, const char * bin, int execonly)
{
    apportable self;
    char * sep;
    char * pathbuf;
    char * pathlim;
    char * path;
    size_t bin_l;
    char * cand;
    size_t cand_l;
    wchar_t * wcand;
#if defined _WIN32
    size_t wcand_l;
#endif
    int filetest;

    self = APPORTABLE_STATE(a);
    if (!self->enabled)
        return NULL;

    pathbuf = (self->_strndup(self, searchpath, 0));
    pathlim = pathbuf + strlen(pathbuf);
    path = pathbuf;
    bin_l = strlen(bin);

    while (path < pathlim) {
        sep = strchr(path, PATHSEP_C);
        if (!sep)
            sep = pathlim;
        *sep = 0;
        /* construct candidate binary path */
        cand_l = strlen(path) + strlen(DIRSEP_S) + bin_l + 1;
        if (!(cand = self->_calloc(1, cand_l)))
        {
            self->_free(pathbuf);
            return NULL;
        }
        strcpy(cand, path);
        strcat(cand, DIRSEP_S);
        strcat(cand, bin);
        cand[cand_l - 1] = 0;
#if !defined _WIN32
        wcand = NULL;
        filetest = execonly ? X_OK : F_OK;
        if (access(cand, filetest) == -1)
#else
        wcand_l = MultiByteToWideChar(CP_UTF8, 0, cand, cand_l, NULL, 0);
        wcand = self->_calloc(sizeof(wchar_t), wcand_l + 0);
        MultiByteToWideChar(CP_UTF8, 0, cand, cand_l, wcand, wcand_l);
        if (_waccess_s(wcand, 04) != 0)
#endif
        {
            if (wcand)
                self->_free(wcand);
            path = sep < pathlim ? sep + 1 : pathlim;
            self->_free(cand);
            continue;
        }
        /* found a candidate */
        self->_free(pathbuf);
        return cand;
    }
    free(pathbuf);
    return self->_strndup(self, bin, 0);
}



char * apportable_pathexp(apportable a, const char * template, const char * library_path)
{
    apportable self;
    char * exec_path_sym;
    unsigned long exec_path_symlen;
    char * result;
    const char * library_name;
    char * executable_path_p;
    char * executable_path;
    unsigned long exec_path_len;
    unsigned long template_len;
    const char * sub_template;
    unsigned long sub_template_len; 
    unsigned long result_len;

    self = APPORTABLE_STATE(a);
    if (!self->enabled)
        return NULL;

    if (!library_path || !template)
        return NULL;
 
    exec_path_sym = "$ORIGIN";
    exec_path_symlen = strlen(exec_path_sym);
    result = NULL;
    
    /* if not starting with $ORIGIN, return straight away a copy of the template, unaltered */
    if (strncmp(template, exec_path_sym, exec_path_symlen) != 0) {
        result = self->_strndup(a, template, 0);
        return result;
    }

    library_name = strrchr(library_path, DIRSEP_C);
    library_name = library_name ? library_name + 1 : library_path;
    if (library_name - library_path != 0) {
        executable_path_p = self->_strndup(a, library_path, library_name - library_path);
        executable_path = executable_path_p;
    } else {
        executable_path_p = NULL;
        executable_path = "." DIRSEP_S;
    }

    exec_path_len = strlen(executable_path);
    template_len = strlen(template);
    sub_template = &template[exec_path_symlen];
    while (sub_template[0] == DIRSEP_C && executable_path[0] == DIRSEP_C)
        sub_template += 1;
    sub_template_len = template_len - exec_path_symlen;
    result_len = template_len + exec_path_len - exec_path_symlen;

    // concatenate
    result = calloc(1, result_len + 1);
    memcpy(result, executable_path, exec_path_len);                   // "@executable_path"
    memcpy(&result[exec_path_len], sub_template, sub_template_len);   // "/../share/"
    result[result_len] = 0;                                           // (again)

    if (executable_path_p)
        free(executable_path_p);
    return result;
}



#endif /*APPORTABLE*/

