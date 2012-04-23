/**************************************************************************
 *   pbackup.c                                                            *
 *                                                                        *
 *   Copyright (C) 2005-2008 Tom Rune Flo <tom@x86.no>                    *
 *   This program is free software; you can redistribute it and/or modify *
 *   it under the terms of the GNU General Public License as published by *
 *   the Free Software Foundation; either version 2, or (at your option)  *
 *   any later version.                                                   *
 *                                                                        *
 *   This program is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *   GNU General Public License for more details.                         *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program; if not, write to the Free Software          *
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.            *
 *                                                                        *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ftw.h>
#include <dirent.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#define PROG_NAME      "pbackup"
#define PROG_VERSION   "0.1"

enum { SAVE, REST, CHECK };

#ifndef PATH_MAX
#define PATH_MAX  4096
#endif

void die(char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);

  va_end(args);

  fprintf(stderr, "\n");

  exit(EXIT_FAILURE);
}


int report(const char *file)
{
  struct stat st;

  if (lstat(file, &st) == -1) {
    fprintf(stderr, "warning: lstat(): `%s': %s\n", file, strerror(errno));
    return -1;
  }

  fprintf(stdout, "%s%c%d\n", file, '\0', st.st_mode);
  
  return 0;
}


int re_report(const char *file, const struct stat *sb, int flag)
{

  /* skip /proc */
  if (strncmp(file, "/proc", 5) != 0)
    report(file);

  return 0;
}


int recursive(const char *path, int maxfd)
{
  return ftw(path, &re_report, maxfd);
}

void nltrim(char *s)
{
  int i;

  for (i = 0; i < strlen(s); i++)
    if (s[i] == '\n') s[i] = '\0';

  return;
}


void usage(void)
{
  fprintf(stderr, 

  "usage: %s <-S|-R|-C> [options] [file(s)]\n\n"
  "OPTIONS\n"
  "  -S                    generate backup data from file(s), dump to stdout\n"
  "  -R                    restore permissions from backup through stdin\n"
  "  -C                    check current file permissions against backup\n\n"
  "  -r                    be recursive (save mode only)\n"
  "  -v                    be verbose (restore mode only)\n"
  "  -m <num>              max open file descriptors when being recursive (default is 64)\n"
  "  -V                    print version and exit\n\n"
  "EXAMPLES\n"
  "  create backup data:          %s -S -r / > permbackup.db\n"
  "  check permission integrity:  %s -C < permbackup.db\n"
  "  restore from backup:         %s -R < permbackup.db\n\n",

  PROG_NAME, PROG_NAME, PROG_NAME, PROG_NAME

  );

  exit(EXIT_FAILURE);
}


int main(int argc, char **argv)
{
  struct stat       st;
  int               mode                    = -1;
  int               o_rec                   = 0;
  int               o_verbose               = 0;
  int               o_check                 = 0;
  int               o_maxfd                 = 64;
  char              buf[PATH_MAX + 64]; 
  unsigned long     checked                 = 0;
  unsigned long     changed                 = 0;
  unsigned long     errors                  = 0;
  char              *perms                  = NULL;
  int               c, i;

  if (argc < 2)
    usage();

  while ((c = getopt(argc, argv, "SRCrvVm:")) != -1) {
    switch (c) {

      case 'S': 
        mode = SAVE;
        break;

      case 'C': 
        o_check = 1;
      case 'R': 
        mode = REST;
        break;


      case 'r':
        o_rec = 1;
        break;

      case 'v':
        o_verbose = 1;
        break;

      case 'm':
        o_maxfd = atoi(optarg);
        break;

      case 'V':
        printf("%s version %s\n", PROG_NAME, PROG_VERSION);
        return 0;

    }
  } /* while */

  if (mode == -1)
    usage();

  //optind++;

  if (mode == SAVE) {

    while (argv[optind]) {

      if (lstat(argv[optind], &st) == -1)
        fprintf(stderr, "error: lstat(): `%s': %s", argv[optind], strerror(errno));

      if (S_ISDIR(st.st_mode) && o_rec == 1) {
        if (recursive(argv[optind], o_maxfd) == -1)
          die("error: ftw() file tree walk failed (too many file descriptors? try -m)");
      }
      else
        report(argv[optind]);

      optind++;

    }

  } /* SAVE */

  else { /* RESTORE */

    for (;;) {

      memset(buf, '\0', sizeof buf);

      if (fgets(buf, sizeof buf, stdin) == NULL)
        break;

      for (i = 0; i < sizeof buf; i++) {
        if (buf[i] == '\0') {
          perms = buf + i + 1;
          nltrim(perms);
          break;
        }
      }

      if (lstat(buf, &st) == 0) {

        checked++;
        if (st.st_mode != atoi(perms)) {
          if (o_check)
            printf("Check mismatch: `%s': %d vs %d\n", buf, st.st_mode, atoi(perms));
          if (o_verbose && !o_check)
            printf("Restoring permissions for `%s': %d -> %d\n", buf, st.st_mode, atoi(perms));
          if (!o_check)
            chmod(buf, atoi(perms));
          changed++;
        }

      }
      else {

        nltrim(buf);
        fprintf(stderr, "warning: lstat(): `%s': %s\n", buf, strerror(errno));
        errors++;

      }

    } /* for */

    if (o_check)
      printf("%ld files checked, %ld mismatching permissions ", checked, changed);
    else
      printf("%ld files checked, %ld permissions fixed ", checked, changed);
    printf("(%ld errors)\n", errors);

  } /* RESTORE */

  return EXIT_SUCCESS;
}
