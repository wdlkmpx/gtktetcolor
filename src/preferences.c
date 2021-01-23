/* GtkTetcolor
 * Copyright (C) 1999 - 2002 Andrey V. Panov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef USE_GNOME
# include <gnome.h>
# include <gconf/gconf-client.h>
#else
# include <gtk/gtk.h>
#endif
#include <glib/gi18n.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <string.h>
#include <pango/pango.h>

#include "callbacks.h"
#include "interface.h"
#include "main.h"
#include "score.h"
#include "game.h"
#include "preferences.h"
#include "pixmaps.h"

gint cell_width, left_margin, initial_level, use_graykeys, destroy_delay;
#ifdef USE_GNOME
gboolean sound_on;
extern GConfClient *gconf_client;
#else
gboolean text_toolbar;
#endif
gint nav_keys[4], alt_nav_keys[4];
GtkStyle *label_style;
gchar *font_name;
gchar *label_name[MAX_LABEL];	/* Label identifiers */

gboolean preferences_changed;

void
change_preferences (GtkWidget * widget)
{
  GtkWidget *cell_size_spinbutton, *initial_level_spinbutton,
    *destroy_delay_spinbutton;
  GtkWidget *fontselection;
  gchar *new_font_name = NULL;
  gint temp;

  cell_size_spinbutton =
    (GtkWidget *) g_object_get_data (G_OBJECT (widget), "cell_size_spinbutton");
  initial_level_spinbutton =
    (GtkWidget *) g_object_get_data (G_OBJECT (widget),
				     "initial_level_spinbutton");
  destroy_delay_spinbutton =
    (GtkWidget *) g_object_get_data (G_OBJECT (widget),
				     "destroy_delay_spinbutton");
  fontselection =
    (GtkWidget *) g_object_get_data (G_OBJECT (widget), "fontselection");
  temp =
    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (cell_size_spinbutton));
  if (temp != cell_width) {
    cell_width = temp;
    create_pixmaps (drawingarea, 0);
    redraw_all_cells ();
    preferences_changed = TRUE;
  }
  temp =
    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
				      (initial_level_spinbutton));
  if (temp != initial_level) {
    initial_level = temp;
    preferences_changed = TRUE;
  }
  temp =
    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
				      (destroy_delay_spinbutton));
  if (temp != destroy_delay) {
    destroy_delay = temp;
    preferences_changed = TRUE;
  }
  if (use_graykeys) {
    nav_keys[0] = GDK_Right;
    nav_keys[1] = GDK_Left;
    nav_keys[2] = GDK_Up;
    nav_keys[3] = GDK_Down;
    alt_nav_keys[0] = GDK_Right;
    alt_nav_keys[1] = GDK_Left;
    alt_nav_keys[2] = GDK_Up;
    alt_nav_keys[3] = GDK_Down;
  }
  else {
    nav_keys[0] = GDK_KP_6;
    nav_keys[1] = GDK_KP_4;
    nav_keys[2] = GDK_KP_8;
    nav_keys[3] = GDK_KP_5;
    alt_nav_keys[0] = GDK_KP_Right;
    alt_nav_keys[1] = GDK_KP_Left;
    alt_nav_keys[2] = GDK_KP_Up;
    alt_nav_keys[3] = GDK_KP_Begin;
  }
  new_font_name =
    gtk_font_selection_get_font_name (GTK_FONT_SELECTION (fontselection));
  if (new_font_name) {
    if (font_name == NULL || strcmp (new_font_name, font_name)) {
      if (font_name)
	g_free (font_name);
      font_name = g_strdup (new_font_name);
      if (font_name) {
	change_font_of_labels ();
	preferences_changed = TRUE;
      }
    }
  g_free (new_font_name);
  }
}				/* change_preferences */

void
change_font_of_labels (void)
{
  GtkWidget *label;
  GtkStyle *newstyle;
  PangoFontDescription *desc;
  gint i, width;
  int height;
  guint max_width = 0;
  char *str;
  PangoLayout *layout = NULL;

  /* Change font */
  if (font_name) {
    desc = pango_font_description_from_string (font_name);
    if (pango_font_description_get_size (desc)) {
      label = (GtkWidget *) g_object_get_data (G_OBJECT (main_window),
					       label_name[0]);
      newstyle = gtk_style_copy (gtk_widget_get_style (label));
      if (newstyle->font_desc != NULL)
	pango_font_description_free (newstyle->font_desc);
      newstyle->font_desc = pango_font_description_copy (desc);
      for (i = 0; i < MAX_LABEL; i++) {
	if (i)
	  label = (GtkWidget *) g_object_get_data (G_OBJECT (main_window),
						   label_name[i]);
	gtk_widget_set_style (label, newstyle);
      }
      g_object_unref (G_OBJECT (newstyle));
    }
    if (desc != NULL)
      pango_font_description_free (desc);
  }
  /* Set proper size for left margin */
  for (i = 2; i <= 4; i *= 2) {
    label = (GtkWidget *) g_object_get_data (G_OBJECT (main_window),
					     label_name[i]);
    str = g_strdup (gtk_label_get_text (GTK_LABEL (label)));
    if (layout)
      g_object_unref (G_OBJECT (layout));
    layout = gtk_widget_create_pango_layout (label, str);
    pango_layout_get_pixel_size (layout, &width, &height);
    max_width = max_width > width ? max_width : width;
    if (str)
      g_free (str);
  }
  pango_layout_set_text (layout, _("Bonus"), -1);
  pango_layout_get_pixel_size (layout, &width, NULL);
  max_width = max_width > width ? max_width : width;
  pango_layout_set_text (layout, _("Game over"), -1);
  pango_layout_get_pixel_size (layout, &width, NULL);
  max_width = max_width > width ? max_width : width;
  gtk_widget_set_size_request (label, max_width, height);
  if (layout)
    g_object_unref (G_OBJECT (layout));
}				/* change_font_of_labels */


void
load_preferences ()
{
  int temp;
#ifdef USE_GNOME
  temp =
    gconf_client_get_int (gconf_client, "/apps/gtktetcolor/Options/cell_size",
			  NULL);
  if (temp >= 10 && temp <= MAX_CELL_SIZE)
    cell_width = temp;
  temp =
    gconf_client_get_int (gconf_client,
			  "/apps/gtktetcolor/Options/initial_level", NULL);
  if (temp >= 1 && temp <= 9)
    initial_level = temp;
  temp =
    gconf_client_get_bool (gconf_client,
			   "/apps/gtktetcolor/Options/use_graykeys", NULL);
  if (temp == 1 || temp == 0)
    use_graykeys = temp;
  temp =
    gconf_client_get_bool (gconf_client, "/apps/gtktetcolor/Options/sound_on",
			   NULL);
  if (temp == 1 || temp == 0)
    sound_on = temp;
  temp =
    gconf_client_get_int (gconf_client,
			  "/apps/gtktetcolor/Options/destroy_delay", NULL);
  if (temp > 0 && temp <= MAX_DESTROY_DELAY)
    destroy_delay = temp;
  font_name =
    gconf_client_get_string (gconf_client, "/apps/gtktetcolor/Options/font",
			     NULL);
#else
  FILE *rcfile;
  gchar *str;
  int i;
# ifdef HAVE_GETLINE
  size_t n = 0;
# endif
# if defined HAVE_GETDELIM
#  ifndef HAVE_GETLINE
  size_t n = 0;
#  endif
  gchar *str1;
# endif

# if !defined HAVE_GETDELIM
  str = (gchar *) g_malloc (20);
# endif
  rcfile = fopen (rcfile_path, "r");
  if (rcfile == NULL)
    g_print ("\ncannot open rc file for reading\n");
  else {
    for (i = 0; !feof (rcfile) && i <= 5; i++) {
# if defined HAVE_GETDELIM
      n = 0;
      getdelim (&str1, &n, ' ', rcfile);
      n = strlen (str1);
      str = g_strndup (str1, n - 1);
# else
      fscanf (rcfile, "%s ", str);
# endif
      if (strcmp (str, "cell_size") == 0) {
	fscanf (rcfile, "%d ", &temp);
	if (temp >= 10 && temp <= MAX_CELL_SIZE)
	  cell_width = temp;
      }
      else if (strcmp (str, "initial_level") == 0) {
	fscanf (rcfile, "%d ", &temp);
	if (temp >= 1 && temp <= 9)
	  initial_level = temp;
      }
      else if (strcmp (str, "use_graykeys") == 0) {
	fscanf (rcfile, "%d ", &temp);
	if (temp == 1 || temp == 0)
	  use_graykeys = temp;
      }
      else if (strcmp (str, "destroy_delay") == 0) {
	fscanf (rcfile, "%d ", &temp);
	if (temp >= 0 && temp <= MAX_DESTROY_DELAY)
	  destroy_delay = temp;
      }
      else if (strcmp (str, "text_toolbar") == 0) {
	fscanf (rcfile, "%d ", &temp);
	if (temp == 1 || temp == 0)
	  text_toolbar = temp;
      }
      else if (strcmp (str, "font") == 0) {
	fscanf (rcfile, "%d ", &temp);
	if (temp > 0) {
# ifdef HAVE_GETLINE
	  getline (&font_name, &n, rcfile);
# else
          if (font_name)
	    g_free (font_name);
	  font_name = (gchar *) g_malloc (temp + 1);
	  fgets ((char *) font_name, temp + 1, rcfile);
# endif
	}
      }
# ifdef HAVE_GETLINE
      else {
	n = 0;
	getline (&str, &n, rcfile);	/* read line until the end */
#  if !defined HAVE_GETDELIM
	if (str)
	  g_free (str);
#  endif
      }
# endif
# if defined HAVE_GETDELIM
      if (str)
	g_free (str);
      if (str1)
	free (str1);
# endif
    }
  }
# if !defined HAVE_GETDELIM
  if (str)
    g_free (str);
# endif
#endif
}				/* load_preferences */

void
save_preferences (void)
{
#ifdef USE_GNOME
  gconf_client_set_int (gconf_client, "/apps/gtktetcolor/Options/cell_size",
			cell_width, NULL);
  gconf_client_set_int (gconf_client, "/apps/gtktetcolor/Options/initial_level",
			initial_level, NULL);
  gconf_client_set_bool (gconf_client, "/apps/gtktetcolor/Options/use_graykeys",
			 use_graykeys, NULL);
  gconf_client_set_bool (gconf_client, "/apps/gtktetcolor/Options/sound_on",
			 sound_on, NULL);
  gconf_client_set_int (gconf_client, "/apps/gtktetcolor/Options/destroy_delay",
			destroy_delay, NULL);
  if (font_name)
    gconf_client_set_string (gconf_client, "/apps/gtktetcolor/Options/font",
			     font_name, NULL);
  gconf_client_suggest_sync (gconf_client, NULL);
#else
  FILE *rcfile;
  rcfile = fopen (rcfile_path, "w");
  if (rcfile == NULL)
    g_print ("\ncannot open rc file for writing\n");
  else {
    fprintf (rcfile, "cell_size %d\n", cell_width);
    fprintf (rcfile, "initial_level %d\n", initial_level);
    fprintf (rcfile, "use_graykeys %d\n", use_graykeys);
    fprintf (rcfile, "destroy_delay %d\n", destroy_delay);
    fprintf (rcfile, "text_toolbar %d\n", text_toolbar);
    if (font_name)
      fprintf (rcfile, "font %d %s\n", strlen (font_name), font_name);
    fclose (rcfile);
  }
#endif
}