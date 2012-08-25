/*
 * Copyright © 2010, 2011 Intel Corporation.
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

#include "guaca-system.h"

#include <guacamayo-version.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <mex/mex.h>
#include <mex/mex-info-bar-component.h>

struct SystemInfo
{
  char  *total_memory;
  char  *free_memory;
  guint  cores;
  char  *cpu_model;
  char  *hostname;
};

free_system_info (struct SystemInfo *info)
{
  g_free (info->total_memory);
  g_free (info->free_memory);
  g_free (info->cpu_model);
  g_free (info->hostname);
  g_slice_free (struct SystemInfo, info);
}

static struct SystemInfo *
get_system_info (void)
{
  struct SystemInfo *info = g_slice_new0 (struct SystemInfo);
  FILE *f;
  char buf[LINE_MAX];

  if ((f = fopen ("/proc/meminfo", "r")))
    {
      while (fgets(buf, sizeof (buf), f))
        {
          if (!strncmp ("MemTotal:", buf, strlen ("MemTotal:")))
            {
              double m  = (double) strtol (buf+strlen ("MemTotal:"), NULL, 10);
              const char *u = "KB";

              if (m > 1024*1024)
                {
                  m /= (1024*1024);
                  u = "GB";
                }
              else if (m > 1024)
                {
                  m /= 1024;
                  u = "MB";
                }

              info->total_memory = g_strdup_printf ("%.2f %s", m, u);

              if (info->free_memory)
                break;
            }
          else if (!strncmp ("MemFree:", buf, strlen ("MemFree:")))
            {
              double m  = (double) strtol (buf+strlen ("MemFree:"), NULL, 10);
              const char *u = "KB";

              if (m > 1024*1024)
                {
                  m /= (1024*1024);
                  u = "GB";
                }
              else if (m > 1024)
                {
                  m /= 1024;
                  u = "MB";
                }

              info->free_memory = g_strdup_printf ("%.2f %s", m, u);

              if (info->total_memory)
                break;
            }
        }

      fclose (f);
    }

  if ((f = fopen ("/proc/cpuinfo", "r")))
    {
      while (fgets(buf, sizeof (buf), f))
        {
          if (!strncmp ("processor", buf, strlen ("processor")))
            {
              info->cores++;
            }
          else if (!info->cpu_model &&
                   !strncmp ("model name", buf, strlen ("model name")))
            {
              char *t;
              int i;

              struct _subs
              {
                const char *match;
                const char *subst;
                guint       c_flags;
                guint       m_flags;
              } subs[] =

                {
                  {"\\s*:\\s*(.*)\n*\r*$",    "\\1", 0, 0},
                  {"cpu",                     "",    G_REGEX_CASELESS, 0},
                  {"\\s{2,}",                 " ",   0, 0},
                  {"\\(R\\)",                 "®",   G_REGEX_CASELESS, 0},
                  {"\\(C\\)",                 "©",   G_REGEX_CASELESS, 0},
                  {"\\(TM\\)",                "™",   G_REGEX_CASELESS, 0},
                };

              t = g_strdup (buf + strlen ("model name"));
              for (i = 0; i < G_N_ELEMENTS(subs); ++i)
                {
                  GRegex     *r;
                  char       *n;

                  r = g_regex_new (subs[i].match, subs[i].c_flags, 0, NULL);
                  n = g_regex_replace (r, t, -1, 0,
                                       subs[i].subst, subs[i].m_flags, NULL);
                  g_regex_unref (r);

                  if (!n)
                    {
                      break;
                    }
                  else
                    {
                      g_free (t);
                      t = n;
                    }
                }

              info->cpu_model = t;
            }
        }

      fclose (f);
    }

  if (!gethostname (buf, sizeof (buf)))
    {
      buf[sizeof (buf) - 1] = 0;
      info->hostname = g_strdup (buf);
    }

  return info;
}

static void mex_info_bar_component_iface_init (MexInfoBarComponentIface *iface);
static void guaca_system_dispose (GObject *object);
static void guaca_system_finalize (GObject *object);

G_DEFINE_TYPE_WITH_CODE (GuacaSystem, guaca_system, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_INFO_BAR_COMPONENT,
                                            mex_info_bar_component_iface_init));

#define GUACA_SYSTEM_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), GUACA_TYPE_SYSTEM, GuacaSystemPrivate))

struct _GuacaSystemPrivate
{
  ClutterActor *button;
  ClutterActor *dialog;
  ClutterActor *transient_for;
  ClutterActor *entry;

  char         *hostname;

  guint disposed : 1;
};

static void
guaca_system_class_init (GuacaSystemClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  g_type_class_add_private (klass, sizeof (GuacaSystemPrivate));

  object_class->dispose      = guaca_system_dispose;
  object_class->finalize     = guaca_system_finalize;
}

static void
guaca_system_init (GuacaSystem *self)
{
  self->priv = GUACA_SYSTEM_GET_PRIVATE (self);
}

static void
guaca_system_dispose (GObject *object)
{
  GuacaSystem        *self = (GuacaSystem*) object;
  GuacaSystemPrivate *priv = self->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  G_OBJECT_CLASS (guaca_system_parent_class)->dispose (object);
}

static void
guaca_system_finalize (GObject *object)
{
  GuacaSystem        *self = (GuacaSystem*) object;
  GuacaSystemPrivate *priv = self->priv;

  g_free (priv->hostname);

  G_OBJECT_CLASS (guaca_system_parent_class)->finalize (object);
}

static MexInfoBarLocation
guaca_system_get_location (MexInfoBarComponent *comp)
{
  return MEX_INFO_BAR_LOCATION_SETTINGS;
}

static int
guaca_system_get_location_index (MexInfoBarComponent *comp)
{
  return -1;
}

static void
guaca_system_close_cb (void *dialog, GuacaSystem *self)
{
  GuacaSystemPrivate *priv = self->priv;

  g_print ("%s:%d\n", __FUNCTION__, __LINE__);

  mex_push_focus (MX_FOCUSABLE (priv->button));
}

static gboolean
guaca_system_close_dialog_cb (MxAction *unused, GuacaSystem *self)
{
  GuacaSystemPrivate *priv = self->priv;
  ClutterActor       *parent;
  const char         *hostname;

  hostname = mx_entry_get_text (MX_ENTRY (priv->entry));
  if (hostname && *hostname && g_strcmp0 (hostname, priv->hostname))
    {
      GError *error = NULL;
      char   *cmd   = g_strdup_printf ("guacamayo-hostname %s", hostname);

      /*
       * Hostname changed, try to set.
       */
      if (!g_spawn_command_line_async (cmd, &error))
        {
          g_warning ("Failed to execute '%s': %", cmd, error->message);
          g_clear_error (&error);
        }

      g_free (cmd);
    }

  parent = clutter_actor_get_parent (priv->dialog);
  clutter_actor_remove_child (parent, priv->dialog);
  priv->dialog = NULL;

  mex_push_focus (MX_FOCUSABLE (priv->button));

  return FALSE;
}

static gboolean
guaca_system_key_press_cb (ClutterActor    *actor,
                           ClutterKeyEvent *event,
                           GuacaSystem     *self)
{
  if (MEX_KEY_BACK (event->keyval))
    {
      guaca_system_close_dialog_cb (NULL, self);
      return TRUE;
    }

  return FALSE;
}

static void
guaca_system_activated_cb (MxAction *action, GuacaSystem *self)
{
  GuacaSystemPrivate *priv = self->priv;
  struct SystemInfo  *info = get_system_info ();
  ClutterActor       *dialog, *layout, *label, *entry;
  MxAction           *close;
  char               *text;
  int                 row = 0;

  dialog = mx_dialog_new ();
  mx_stylable_set_style_class (MX_STYLABLE (dialog), "MexInfoBarDialog");

  close = mx_action_new_full ("close", _("Close"),
                              G_CALLBACK (guaca_system_close_dialog_cb), self);
  mx_dialog_add_action (MX_DIALOG (dialog), close);

  layout = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (layout), 10);
  mx_table_set_row_spacing (MX_TABLE (layout), 20);
  mx_bin_set_child (MX_BIN (dialog), layout);

  label = mx_label_new_with_text (_("System Settings"));
  mx_stylable_set_style_class (MX_STYLABLE (label), "DialogHeader");
  mx_table_insert_actor (MX_TABLE (layout), label, row++, 0);

  label = mx_label_new_with_text (_("Device name:"));
  mx_table_insert_actor (MX_TABLE (layout), label, row, 0);
  priv->entry = entry = mx_entry_new_with_text (info->hostname);
  g_free (priv->hostname);
  priv->hostname = g_strdup (info->hostname);

  mx_table_insert_actor (MX_TABLE (layout), entry, row++, 1);

  label = mx_label_new_with_text (_("Software:"));
  mx_table_insert_actor (MX_TABLE (layout), label, row, 0);
  label = mx_label_new_with_text (GUACAMAYO_DISTRO_STRING);
  mx_table_insert_actor (MX_TABLE (layout), label, row++, 1);

  label = mx_label_new_with_text (_("Processor:"));
  mx_table_insert_actor (MX_TABLE (layout), label, row, 0);
  label = mx_label_new_with_text (info->cpu_model);
  mx_table_insert_actor (MX_TABLE (layout), label, row++, 1);

  label = mx_label_new_with_text (_("Cores:"));
  mx_table_insert_actor (MX_TABLE (layout), label, row, 0);
  text = g_strdup_printf ("%d", info->cores);
  label = mx_label_new_with_text (text);
  g_free (text);
  mx_table_insert_actor (MX_TABLE (layout), label, row++, 1);

  label = mx_label_new_with_text (_("Memory:"));
  mx_table_insert_actor (MX_TABLE (layout), label, row, 0);
  text = g_strdup_printf (_("%s (free %s)"),
                          info->total_memory, info->free_memory);
  label = mx_label_new_with_text (text);
  g_free (text);
  mx_table_insert_actor (MX_TABLE (layout), label, row++, 1);

  mx_dialog_set_transient_parent (MX_DIALOG (dialog), priv->transient_for);
  g_signal_connect (dialog, "key-press-event",
                    G_CALLBACK (guaca_system_key_press_cb), self);

  priv->dialog = dialog;

  clutter_actor_show (dialog);
  mex_push_focus (MX_FOCUSABLE (dialog));

  free_system_info (info);
}

static ClutterActor *
guaca_system_create_ui (MexInfoBarComponent *comp,
                        ClutterActor        *transient_for)
{
  GuacaSystem  *self;
  ClutterActor *graphic, *tile, *button;
  gchar        *tmp;
  MxAction     *action;

  g_return_val_if_fail (GUACA_IS_SYSTEM (comp), NULL);

  self = GUACA_SYSTEM (comp);

  /*
   * The actual dialog is being constructed on demand, so store the
   * transient parent.
   */
  self->priv->transient_for = transient_for;

  /*
   * Make the button for the Settings dialog.
   */
  graphic = mx_image_new ();
  mx_stylable_set_style_class (MX_STYLABLE (graphic),
                               "GuacaSystemGraphic");

  tmp = g_build_filename (mex_get_data_dir (), "style",
                          "graphic-system.png", NULL);
  mx_image_set_from_file (MX_IMAGE (graphic), tmp, NULL);
  g_free (tmp);

  tile = mex_tile_new ();
  mex_tile_set_label (MEX_TILE (tile), _("System"));
  mex_tile_set_important (MEX_TILE (tile), TRUE);

  button = mx_button_new ();

  action = mx_action_new_full ("guaca-system-settings",
                               _("System Settings"),
                               G_CALLBACK (guaca_system_activated_cb),
                               self);

  mx_button_set_action (MX_BUTTON (button), action);

  mx_bin_set_child (MX_BIN (tile), button);
  mx_bin_set_child (MX_BIN (button), graphic);

  self->priv->button = tile;

  return tile;
}

static void
mex_info_bar_component_iface_init (MexInfoBarComponentIface *iface)
{
  iface->get_location       = guaca_system_get_location;
  iface->get_location_index = guaca_system_get_location_index;
  iface->create_ui          = guaca_system_create_ui;
}

static GType
guaca_system_plugin_get_type (void)
{
  return GUACA_TYPE_SYSTEM;
}

MEX_DEFINE_PLUGIN ("System",
		   "System Settings",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "Tomas Frydrych <tomas@sleepfive.com>",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   guaca_system_plugin_get_type,
		   MEX_PLUGIN_PRIORITY_NORMAL)
