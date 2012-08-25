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

/*
 * Host name setter for guacamayo settings mex plugin
 *
 * This program needs to be installed suid root
 */
int
main (int argc, char **argv)
{
  int   i, j, len, retval = 0;
  char *n;

  if (argc != 2)
    return 1;

  n = argv[1];

  /*
   * Strip control chars
   */
  len = strlen (n);
  for (i = 0, j = 0; i < len; i++)
    if (n[i] >= ' ')
      n[j++] = n[i];

  n[j] = 0;

  if (sethostname (n, j))
    {
      fprintf (stderr, "Failed to set hostname to '%s': %s\n",
               n, strerror (errno));
      retval = 1;
    }
  else
    {
      FILE *f;

      /*
       * The change made by sethostname() is not persistent, since at bootime
       * the hostname is read from /etc/hostname, so fix that too.
       */
      printf ("Host name set to '%s'\n", n);

      if (!(f = fopen ("/etc/hostname", "w")))
        {
          fprintf (stderr, "Failed to set save hostname: %s\n",
                   strerror (errno));
          retval = 2;
        }
      else
        {
          if (fwrite (n, j, 1, f) != 1)
            {
              fprintf (stderr, "Failed to set save hostname: %s\n",
                       strerror (errno));
              retval = 2;
            }

          fclose (f);
        }
    }

  return retval;
}
