/*
 * Copyright © 2012, sleep(5) ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>

/*
 * Timezone setter for guacamayo clock settings mex plugin
 *
 * This program needs to be installed suid root
 */
int
main (int argc, char **argv)
{
  int         retval = 0;
  char       *zone, *p;
  char        path[PATH_MAX];
  FILE       *f = NULL;
  struct stat st;

  if (argc != 2)
    return 1;

  zone = argv[1];

  snprintf (path, sizeof(path), "/usr/share/zoneinfo/%s", zone);

  if (stat (path, &st) < 0)
    {
      fprintf (stderr, "Failed to stat '%s': %s\n", zone, strerror (errno));
      retval = 2;
      goto finish;
    }

  if ((f = fopen ("/etc/timezone", "w")))
    {
      fputs (zone, f);
    }
  else
    {
      fprintf (stderr, "Failed to open /etc/timezone: %s", strerror (errno));
      retval = 3;
      goto finish;
    }

  if (unlink ("/etc/localtime"))
    {
      fprintf (stderr, "Failed to unlink localtime: %s", strerror (errno));
      retval = 4;
      goto finish;
    }


  if (symlink (path, "/etc/localtime"))
    {
      fprintf (stderr, "Failed to symlink local time: %s", strerror (errno));
      retval = 5;
      goto finish;
    }

 finish:
  if (f)
    fclose (f);

  return retval;
}
