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

/* Guacamayo System Settings Mex plugin */

#ifndef __GUACA_SYSTEM_H__
#define __GUACA_SYSTEM_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GUACA_TYPE_SYSTEM (guaca_system_get_type())
#define GUACA_SYSTEM(obj)                                        \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                            \
                               GUACA_TYPE_SYSTEM,                \
                               GuacaSystem))
#define GUACA_SYSTEM_CLASS(klass)                                \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                             \
                            GUACA_TYPE_SYSTEM,                   \
                            GuacaSystemClass))
#define GUACA_IS_SYSTEM(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                            \
                               GUACA_TYPE_SYSTEM))
#define GUACA_IS_SYSTEM_CLASS(klass)                             \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                             \
                            GUACA_TYPE_SYSTEM))
#define GUACA_SYSTEM_GET_CLASS(obj)                              \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                             \
                              GUACA_TYPE_SYSTEM,                 \
                              GuacaSystemClass))

typedef struct _GuacaSystem        GuacaSystem;
typedef struct _GuacaSystemClass   GuacaSystemClass;
typedef struct _GuacaSystemPrivate GuacaSystemPrivate;

struct _GuacaSystemClass
{
  GObjectClass parent_class;
};

struct _GuacaSystem
{
  GObject parent;

  /*<private>*/
  GuacaSystemPrivate *priv;
};

GType guaca_system_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GUACA_SYSTEM_H__ */
