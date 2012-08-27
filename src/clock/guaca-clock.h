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

/* Guacamayo Clock Settings Mex plugin */

#ifndef __GUACA_CLOCK_H__
#define __GUACA_CLOCK_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GUACA_TYPE_CLOCK (guaca_clock_get_type())
#define GUACA_CLOCK(obj)                                        \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                           \
                               GUACA_TYPE_CLOCK,                \
                               GuacaClock))
#define GUACA_CLOCK_CLASS(klass)                                \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                            \
                            GUACA_TYPE_CLOCK,                   \
                            GuacaClockClass))
#define GUACA_IS_CLOCK(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                           \
                               GUACA_TYPE_CLOCK))
#define GUACA_IS_CLOCK_CLASS(klass)                             \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                            \
                            GUACA_TYPE_CLOCK))
#define GUACA_CLOCK_GET_CLASS(obj)                              \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                            \
                              GUACA_TYPE_CLOCK,                 \
                              GuacaClockClass))

typedef struct _GuacaClock        GuacaClock;
typedef struct _GuacaClockClass   GuacaClockClass;
typedef struct _GuacaClockPrivate GuacaClockPrivate;

struct _GuacaClockClass
{
  GObjectClass parent_class;
};

struct _GuacaClock
{
  GObject parent;

  /*<private>*/
  GuacaClockPrivate *priv;
};

GType guaca_clock_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GUACA_CLOCK_H__ */
