/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpdrawableselect_pdb.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

/* NOTE: This file is auto-generated by pdbgen.pl */

#include "config.h"

#include "stamp-pdbgen.h"

#include "gimp.h"


/**
 * SECTION: gimpdrawableselect
 * @title: gimpdrawableselect
 * @short_description: Methods of a drawable chooser dialog
 *

 **/


/**
 * gimp_drawables_popup:
 * @callback: The callback PDB proc to call when user chooses an drawable.
 * @popup_title: Title of the drawable selection dialog.
 * @drawable_type: The name of the GIMP_TYPE_DRAWABLE subtype.
 * @initial_drawable: The drawable to set as the initial choice.
 * @parent_window: An optional parent window handle for the popup to be set transient to.
 *
 * Invokes the drawable selection dialog.
 *
 * Opens a dialog letting a user choose an drawable.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_drawables_popup (const gchar  *callback,
                      const gchar  *popup_title,
                      const gchar  *drawable_type,
                      GimpDrawable *initial_drawable,
                      GBytes       *parent_window)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gboolean success = TRUE;

  args = gimp_value_array_new_from_types (NULL,
                                          G_TYPE_STRING, callback,
                                          G_TYPE_STRING, popup_title,
                                          G_TYPE_STRING, drawable_type,
                                          GIMP_TYPE_DRAWABLE, initial_drawable,
                                          G_TYPE_BYTES, parent_window,
                                          G_TYPE_NONE);

  return_vals = gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                              "gimp-drawables-popup",
                                              args);
  gimp_value_array_unref (args);

  success = GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS;

  gimp_value_array_unref (return_vals);

  return success;
}

/**
 * gimp_drawables_close_popup:
 * @callback: The name of the callback registered for this pop-up.
 *
 * Close the drawable selection dialog.
 *
 * Closes an open drawable selection dialog.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_drawables_close_popup (const gchar *callback)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gboolean success = TRUE;

  args = gimp_value_array_new_from_types (NULL,
                                          G_TYPE_STRING, callback,
                                          G_TYPE_NONE);

  return_vals = gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                              "gimp-drawables-close-popup",
                                              args);
  gimp_value_array_unref (args);

  success = GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS;

  gimp_value_array_unref (return_vals);

  return success;
}

/**
 * gimp_drawables_set_popup:
 * @callback: The name of the callback registered for this pop-up.
 * @drawable: The drawable to set as selected.
 *
 * Sets the selected drawable in a drawable selection dialog.
 *
 * Sets the selected drawable in a drawable selection dialog.
 *
 * Returns: TRUE on success.
 **/
gboolean
gimp_drawables_set_popup (const gchar  *callback,
                          GimpDrawable *drawable)
{
  GimpValueArray *args;
  GimpValueArray *return_vals;
  gboolean success = TRUE;

  args = gimp_value_array_new_from_types (NULL,
                                          G_TYPE_STRING, callback,
                                          GIMP_TYPE_DRAWABLE, drawable,
                                          G_TYPE_NONE);

  return_vals = gimp_pdb_run_procedure_array (gimp_get_pdb (),
                                              "gimp-drawables-set-popup",
                                              args);
  gimp_value_array_unref (args);

  success = GIMP_VALUES_GET_ENUM (return_vals, 0) == GIMP_PDB_SUCCESS;

  gimp_value_array_unref (return_vals);

  return success;
}