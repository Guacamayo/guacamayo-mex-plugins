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

#include "guaca-clock.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <glib/gi18n-lib.h>

#include <gmodule.h>
#include <mex/mex.h>
#include <mex/mex-info-bar-component.h>

#ifndef HAVE_MX_COMBO_BOX_POPULATE
#error You need mx-combo-box-populate.patch from Guacamayo
#endif

static void mex_info_bar_component_iface_init (MexInfoBarComponentIface *iface);
static void guaca_clock_dispose (GObject *object);
static void guaca_clock_finalize (GObject *object);

G_DEFINE_TYPE_WITH_CODE (GuacaClock, guaca_clock, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MEX_TYPE_INFO_BAR_COMPONENT,
                                            mex_info_bar_component_iface_init));

#define GUACA_CLOCK_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), GUACA_TYPE_CLOCK, GuacaClockPrivate))

typedef struct TzEntry
{
  char       *country;
  char       *zone;
  char       *region;
  char       *city;
} TzEntry;

static TzEntry *
tz_entry_new (const char *country, const char *zone)
{
  TzEntry     *t;
  char        *p, *region, *path;
  struct stat  st;

  /*
   * Make sure we have the actual zone info here, since Poky prunes the data
   * without prooning the zones.tab
   */
  path = g_build_filename ("/usr/share/zoneinfo", zone, NULL);

  if (stat (path, &st) < 0)
    {
      g_free (path);
      return NULL;
    }

  g_free (path);

  t = g_slice_new0 (TzEntry);

  t->country = g_strdup (country);
  t->zone    = g_strdup (zone);

  region = g_strdup (zone);

  /* replace underscores with spaces */
  for (p = region; *p; p++)
    if (*p == '_')
      *p = ' ';

  if ((p = strchr (region, '/')))
    {
      *p = 0;
      t->city = g_strdup (_(p+1));
    }

  t->region = g_strdup (_(region));

  g_free (region);

  return t;
}

static void
tz_entry_free (TzEntry *t)
{
  g_free (t->country);
  g_free (t->zone);
  g_free (t->region);
  g_free (t->city);

  g_slice_free (TzEntry, t);
}

static int
tz_entry_cmp (TzEntry *e1, TzEntry *e2)
{
  return g_strcmp0 (e1->zone, e2->zone);
}

struct _GuacaClockPrivate
{
  ClutterActor *button;
  ClutterActor *dialog;
  ClutterActor *transient_for;
  ClutterActor *regions_combo;
  ClutterActor *city_combo;

  char         *orig_zone;

  GHashTable   *regions;

  guint disposed : 1;
};

static void
guaca_clock_get_zones (GuacaClock *self)
{
  GuacaClockPrivate *priv = self->priv;
  FILE              *f;
  char               buf[512];
  GList             *regions;
  GList             *l;

  if (!(f = fopen ("/usr/share/zoneinfo/zone.tab", "r")))
    {
      g_warning ("Failed to open zone.tab: %s", strerror (errno));
      return;
    }

  while (fgets (buf, sizeof (buf), f))
    {
      char    *code, *coords, *zone;
      TzEntry *e;

      if (buf[0] == '#')
        continue;

      buf[sizeof (buf)-1] = 0;

      if (! (code = strtok (buf, "\t\n")))
        continue;
      if (! (coords = strtok (NULL, "\t\n")))
        continue;
      if (! (zone = strtok (NULL, "\t\n")))
        continue;

      /*
       * Push this into a hash table keyed by region (the MxComboBox is too
       * inefficient to manage big lists, and it would be user unfriendly anyway
       */
      if (!(e = tz_entry_new (code, zone)))
        continue;

      l = g_hash_table_lookup (priv->regions, e->region);
      l = g_list_prepend (l, e);
      g_hash_table_insert (priv->regions, e->region, l);
    }

  fclose (f);

  /*
   * Now sort out the individual sublists
   */
  regions = g_hash_table_get_values (priv->regions);
  for (l = regions; l; l = l->next)
    {
      GList   *k = l->data;
      TzEntry *e = k->data;

      k = g_list_sort (k, (GCompareFunc)tz_entry_cmp);
      g_hash_table_insert (priv->regions, e->region, k);
    }
  g_list_free (regions);
}

static void
guaca_clock_class_init (GuacaClockClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  g_type_class_add_private (klass, sizeof (GuacaClockPrivate));

  object_class->dispose  = guaca_clock_dispose;
  object_class->finalize = guaca_clock_finalize;
}

static void
free_hash_list (GList *l)
{
  g_list_free_full (l, (GDestroyNotify) tz_entry_free);
}

static void
guaca_clock_init (GuacaClock *self)
{
  self->priv = GUACA_CLOCK_GET_PRIVATE (self);

  /*
   * region-keyed hashtable holding GList's of TzEntries.
   * We can supply destroy functions here because (a) the key itself is stored
   * inside one of the TzEntries, and (b) we need to be able to replace the
   * data for each key without destroying the list when we are generating the
   * whole TZ database.
   */
  self->priv->regions = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
guaca_clock_dispose (GObject *object)
{
  GuacaClock        *self = (GuacaClock*) object;
  GuacaClockPrivate *priv = self->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  G_OBJECT_CLASS (guaca_clock_parent_class)->dispose (object);
}

static void
guaca_clock_finalize (GObject *object)
{
  GuacaClock        *self = (GuacaClock*) object;
  GuacaClockPrivate *priv = self->priv;
  GList             *values;

  g_free (priv->orig_zone);

  /*
   * Manually destroy the hash table contents; see comment in _init() why.
   */
  values = g_hash_table_get_values (priv->regions);
  g_list_free_full (values, (GDestroyNotify)free_hash_list);
  g_hash_table_destroy (priv->regions);

  G_OBJECT_CLASS (guaca_clock_parent_class)->finalize (object);
}

static MexInfoBarLocation
guaca_clock_get_location (MexInfoBarComponent *comp)
{
  return MEX_INFO_BAR_LOCATION_SETTINGS;
}

static int
guaca_clock_get_location_index (MexInfoBarComponent *comp)
{
  return -1;
}

static void
guaca_clock_close_cb (void *dialog, GuacaClock *self)
{
  GuacaClockPrivate *priv = self->priv;

  mex_push_focus (MX_FOCUSABLE (priv->button));
}

/*
 * Get the timezone string representing the current selection in our
 * combo boxes.
 */
static const char *
guaca_clock_get_current_zone (GuacaClock *self)
{
  GuacaClockPrivate *priv = self->priv;
  TzEntry           *e;
  int                i_c;
  const char        *region;
  GList             *l;

  if ((i_c = mx_combo_box_get_index (MX_COMBO_BOX (priv->city_combo))) < 0)
    return NULL;

  if (mx_combo_box_get_index (MX_COMBO_BOX (priv->city_combo)) < 0)
    return NULL;

  region = mx_combo_box_get_active_text (MX_COMBO_BOX (priv->regions_combo));
  if ((l = g_hash_table_lookup (priv->regions, region)))
    {
      TzEntry *e;

      if ((e = g_list_nth_data (l, i_c)))
        return e->zone;
      else
        g_warning ("No TzEntry for current city selection '%s'",
                   mx_combo_box_get_active_text (MX_COMBO_BOX (
                                                          priv->city_combo)));
    }
  else
    g_warning ("No hashtable entry for region '%s'", region);

  return NULL;
}

static void
guaca_clock_dialog_mapped_cb (ClutterActor *dialog,
                              GParamSpec   *pspec,
                              GuacaClock   *self)
{
  GuacaClockPrivate *priv = self->priv;
  ClutterActor      *parent;

  parent = clutter_actor_get_parent (priv->dialog);
  clutter_actor_remove_child (parent, priv->dialog);
  priv->dialog = NULL;
}

static gboolean
guaca_clock_close_dialog_cb (MxAction *unused, GuacaClock *self)
{
  GuacaClockPrivate *priv = self->priv;
  const char        *zone;

  if ((zone = guaca_clock_get_current_zone (self)) &&
      g_strcmp0 (zone, priv->orig_zone))
    {
      GError *error = NULL;
      char   *cmd   = g_strdup_printf ("guacamayo-timezone %s", zone);

      if (!g_spawn_command_line_async (cmd, &error))
        {
          g_warning ("Failed to execute '%s': %s", cmd, error->message);
          g_clear_error (&error);
        }

      g_free (cmd);
    }

  /*
   * Hide the dialog, this will trigger the default transition; once it is
   * no longer visible, destroy it.
   *
   * We have to use the mapped property here; MxDialog runs custom animation
   * using a timeline and the 'visible' property is set well before the
   * animation finishes.
   */
  g_signal_connect (priv->dialog, "notify::mapped",
                    G_CALLBACK (guaca_clock_dialog_mapped_cb), self);
  clutter_actor_hide (priv->dialog);
  mex_push_focus (MX_FOCUSABLE (priv->button));

  return FALSE;
}

static gboolean
guaca_clock_key_press_cb (ClutterActor    *actor,
                          ClutterKeyEvent *event,
                          GuacaClock      *self)
{
  if (MEX_KEY_BACK (event->keyval))
    {
      guaca_clock_close_dialog_cb (NULL, self);
      return TRUE;
    }

  return FALSE;
}

/*
 * Callback for when the selection in the Region combo changes.
 */
static void
guaca_clock_regions_index_cb (MxComboBox *combo,
                              GParamSpec *pspec,
                              GuacaClock *self)
{
  GuacaClockPrivate *priv = self->priv;
  int                idx, i;
  const char        *text;
  GList             *l;
  char              *region;
  const char        *city = NULL;
  gboolean           orig_region = FALSE;
  GArray            *cities;

  if (((idx = mx_combo_box_get_index (combo)) < 0) ||
      !(text = mx_combo_box_get_active_text (combo)))
    return;

  if (!(l = g_hash_table_lookup (priv->regions, text)))
    {
      g_warning ("No hashtable entry for '%s'", text);
      return;
    }

  if (priv->orig_zone)
    {
      char *p;

      region = g_strdup (priv->orig_zone);
      if ((p = strchr (region, '/')))
        {
          *p = 0;
          city = p + 1;
        }

      if (!g_strcmp0 (region, text))
        orig_region = TRUE;
    }

  mx_combo_box_remove_all (MX_COMBO_BOX (priv->city_combo));
  idx = -1;

  cities = g_array_sized_new (TRUE, FALSE, sizeof (char *), 150);

  for (i = 0; l; l = l->next, i++)
    {
      TzEntry *e = l->data;

      if (orig_region && !g_strcmp0 (e->city, city))
        idx = i;

      g_array_append_val (cities, e->city);
    }

  if (cities->len)
    mx_combo_box_populate (MX_COMBO_BOX (priv->city_combo),
                           (const char **)cities->data);

  g_array_unref (cities);

  if (idx >= 0)
    mx_combo_box_set_index (MX_COMBO_BOX (priv->city_combo), idx);
  else
    mx_combo_box_set_index (MX_COMBO_BOX (priv->city_combo), 0);

  clutter_actor_show (priv->city_combo);
  g_free (region);
}

static void
guaca_clock_activated_cb (MxAction *action, GuacaClock *self)
{
  GuacaClockPrivate *priv = self->priv;
  ClutterActor      *dialog, *layout, *label;
  MxAction          *close;
  char              *text;
  int                row = 0;
  GList             *l, *keys;
  GArray            *regions;
  FILE              *f;
  char               buf[512];

  /*
   * Get the current zone
   */
  if ((f = fopen ("/etc/timezone", "r")) &&
      fgets (buf, sizeof (buf), f))
    {
      char *n;

      /* ensure 0-terminated, strip trailing \n */
      buf[sizeof (buf)-1] = 0;

      if ((n = strchr (buf, '\n')))
        *n = 0;

      g_free (priv->orig_zone);
      priv->orig_zone = g_strdup (buf);
      fclose (f);
    }
  else
    g_warning ("Failed to open /etc/timezone: %s", strerror (errno));

  dialog = mx_dialog_new ();
  mx_stylable_set_style_class (MX_STYLABLE (dialog), "MexInfoBarDialog");

  close = mx_action_new_full ("close", _("Close"),
                              G_CALLBACK (guaca_clock_close_dialog_cb), self);
  mx_dialog_add_action (MX_DIALOG (dialog), close);

  layout = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (layout), 10);
  mx_table_set_row_spacing (MX_TABLE (layout), 20);
  mx_bin_set_child (MX_BIN (dialog), layout);

  label = mx_label_new_with_text (_("Clock Settings"));
  mx_stylable_set_style_class (MX_STYLABLE (label), "DialogHeader");
  mx_table_insert_actor (MX_TABLE (layout), label, row++, 0);

  label = mx_label_new_with_text (_("Timezone:"));
  mx_table_insert_actor (MX_TABLE (layout), label, row, 0);
  priv->regions_combo = mx_combo_box_new ();
  priv->city_combo = mx_combo_box_new ();
  clutter_actor_hide (priv->city_combo);

  guaca_clock_get_zones (self);
  keys = g_hash_table_get_keys (priv->regions);
  keys = g_list_sort (keys, g_strcmp0);

  regions = g_array_sized_new (TRUE, FALSE, sizeof (char *), 15);

  for (l = keys; l; l = l->next)
    {
      const char *region = l->data;

      g_array_append_val (regions, region);
    }

  if (regions->len)
    mx_combo_box_populate (MX_COMBO_BOX (priv->regions_combo),
                           (const char **)regions->data);

  g_array_unref (regions);

  /*
   * Only now that the regions combo is populated we connect to the index
   * notification.
   */
  g_signal_connect (priv->regions_combo, "notify::index",
                    G_CALLBACK (guaca_clock_regions_index_cb),
                    self);

  /*
   * Select the current region in the Regions combo (this in turn will
   * trigger the city combo callback).
   */
  if (priv->orig_zone)
    {
      GList *l;
      int    i;
      char   *region = g_strdup (priv->orig_zone);
      char   *p, *city;

      if ((p = strchr (region, '/')))
        {
          *p = 0;
          city = p + 1;
        }

      for (l = keys, i = 0; l; l = l->next, i++)
        {
          const char *r = l->data;

          if (!g_strcmp0 (r, region))
            {
              mx_combo_box_set_index (MX_COMBO_BOX (priv->regions_combo), i);
              break;
            }
        }

      g_free (region);
    }

  g_list_free (keys);

  mx_table_insert_actor (MX_TABLE (layout), priv->regions_combo, row++, 1);
  mx_table_insert_actor (MX_TABLE (layout), priv->city_combo, row++, 1);

  mx_dialog_set_transient_parent (MX_DIALOG (dialog), priv->transient_for);
  g_signal_connect (dialog, "key-press-event",
                    G_CALLBACK (guaca_clock_key_press_cb), self);

  priv->dialog = dialog;

  clutter_actor_show (dialog);
  mex_push_focus (MX_FOCUSABLE (dialog));
}

static ClutterActor *
guaca_clock_create_ui (MexInfoBarComponent *comp,
                        ClutterActor        *transient_for)
{
  GuacaClock   *self;
  ClutterActor *graphic, *tile, *button;
  gchar        *tmp;
  MxAction     *action;

  g_return_val_if_fail (GUACA_IS_CLOCK (comp), NULL);

  self = GUACA_CLOCK (comp);

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
                               "GuacaClockGraphic");

  tmp = g_build_filename (mex_get_data_dir (), "style",
                          "graphic-clock.png", NULL);
  mx_image_set_from_file (MX_IMAGE (graphic), tmp, NULL);
  g_free (tmp);

  tile = mex_tile_new ();
  mex_tile_set_label (MEX_TILE (tile), _("Clock"));
  mex_tile_set_important (MEX_TILE (tile), TRUE);

  button = mx_button_new ();

  action = mx_action_new_full ("guaca-clock-settings",
                               _("Clock Settings"),
                               G_CALLBACK (guaca_clock_activated_cb),
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
  iface->get_location       = guaca_clock_get_location;
  iface->get_location_index = guaca_clock_get_location_index;
  iface->create_ui          = guaca_clock_create_ui;
}

static GType
guaca_clock_plugin_get_type (void)
{
  return GUACA_TYPE_CLOCK;
}

MEX_DEFINE_PLUGIN ("Clock",
		   "Clock Settings",
		   PACKAGE_VERSION,
		   "LGPLv2.1+",
                   "Tomas Frydrych <tomas@sleepfive.com>",
		   MEX_API_MAJOR, MEX_API_MINOR,
		   guaca_clock_plugin_get_type,
		   MEX_PLUGIN_PRIORITY_NORMAL)
