/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
 *
 * file-save.c
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdocumentlist.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-duplicate.h"
#include "core/gimpimage-scale.h"
#include "core/gimpimagefile.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"

#include "pdb/gimppdb.h"

#include "plug-in/gimppluginprocedure.h"

#include "file-remote.h"
#include "file-save.h"
#include "file-utils.h"
#include "gimp-file.h"

#include "gimp-intl.h"


static GimpImage * image_duplicate_for_export (GimpImage    *image,
                                               GimpProgress *progress);

/*  public functions  */

GimpPDBStatusType
file_save (Gimp                *gimp,
           GimpImage           *image,
           GimpProgress        *progress,
           GFile               *file,
           GimpPlugInProcedure *file_proc,
           GimpRunMode          run_mode,
           gboolean             change_saved_state,
           gboolean             export_backward,
           gboolean             export_forward,
           GError             **error)
{
  GimpImage         *export_image = NULL;
  GimpDrawable      *drawable;
  GimpValueArray    *return_vals;
  GimpPDBStatusType  status     = GIMP_PDB_EXECUTION_ERROR;
  GFile             *local_file = NULL;
  gchar             *path       = NULL;
  gchar             *uri        = NULL;
  gint32             image_ID;
  gint32             drawable_ID;
  GError            *my_error   = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress),
                        GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (G_IS_FILE (file), GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (file_proc),
                        GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail ((export_backward && export_forward) == FALSE,
                        GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (error == NULL || *error == NULL,
                        GIMP_PDB_CALLING_ERROR);

  /*  ref image and file, so they can't get deleted during save  */
  g_object_ref (image);
  g_object_ref (file);

  if (export_forward || export_backward)
    {
      export_image = image_duplicate_for_export (image, progress);
      drawable = gimp_image_get_active_drawable (image);
    }
  else
    {
      drawable = gimp_image_get_active_drawable (image);
    }

  if (! drawable)
    goto out;

  /* FIXME enable these tests for remote files again, needs testing */
  if (g_file_is_native (file) &&
      g_file_query_exists (file, NULL))
    {
      GFileInfo *info;

      info = g_file_query_info (file,
                                G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
                                G_FILE_QUERY_INFO_NONE,
                                NULL, error);
      if (! info)
        goto out;

      if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR)
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Not a regular file"));
          g_object_unref (info);
          goto out;
        }

      if (! g_file_info_get_attribute_boolean (info,
                                               G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE))
        {
          g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("Permission denied"));
          g_object_unref (info);
          goto out;
        }

      g_object_unref (info);
    }

  if (! g_file_is_native (file) &&
      ! file_remote_mount_file (gimp, file, progress, &my_error))
    {
      if (my_error)
        g_propagate_error (error, my_error);
      else
        status = GIMP_PDB_CANCEL;

      goto out;
    }

  if (! file_proc->handles_uri)
    {
      path = g_file_get_path (file);

      if (! path)
        {
          local_file = file_remote_upload_image_prepare (gimp, file, progress,
                                                         &my_error);

          if (! local_file)
            {
              if (my_error)
                g_propagate_error (error, my_error);
              else
                status = GIMP_PDB_CANCEL;

              goto out;
            }

          path = g_file_get_path (local_file);
        }
    }

  if (! path)
    path = g_file_get_uri (file);

  uri = g_file_get_uri (file);

  image_ID    = gimp_image_get_ID (export_image ? export_image : image);
  drawable_ID = gimp_item_get_ID (GIMP_ITEM (drawable));

  return_vals =
    gimp_pdb_execute_procedure_by_name (image->gimp->pdb,
                                        gimp_get_user_context (gimp),
                                        progress, error,
                                        gimp_object_get_name (file_proc),
                                        GIMP_TYPE_INT32,       run_mode,
                                        GIMP_TYPE_IMAGE_ID,    image_ID,
                                        GIMP_TYPE_DRAWABLE_ID, drawable_ID,
                                        G_TYPE_STRING,         path,
                                        G_TYPE_STRING,         uri,
                                        G_TYPE_NONE);

  status = g_value_get_enum (gimp_value_array_index (return_vals, 0));

  gimp_value_array_unref (return_vals);

  if (local_file)
    {
      if (status == GIMP_PDB_SUCCESS)
        {
          GError *my_error = NULL;

          if (! file_remote_upload_image_finish (gimp, file, local_file,
                                                 progress, &my_error))
            {
              status = GIMP_PDB_EXECUTION_ERROR;

              if (my_error)
                g_propagate_error (error, my_error);
              else
                status = GIMP_PDB_CANCEL;
            }
        }

      g_file_delete (local_file, NULL, NULL);
      g_object_unref (local_file);
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpDocumentList *documents;
      GimpImagefile    *imagefile;

      if (change_saved_state)
        {
          gimp_image_set_file (image, file);
          gimp_image_set_save_proc (image, file_proc);

          /* Forget the import source when we save. We interpret a
           * save as that the user is not interested in being able
           * to quickly export back to the original any longer
           */
          gimp_image_set_imported_file (image, NULL);

          gimp_image_clean_all (image);
        }
      else if (export_backward)
        {
          /* We exported the image back to its imported source,
           * change nothing about export/import flags, only set
           * the export state to clean
           */
          gimp_image_export_clean_all (image);

          gimp_object_name_changed (GIMP_OBJECT (image));
        }
      else if (export_forward)
        {
          /* Remember the last entered Export URI for the image. We
           * only need to do this explicitly when exporting. It
           * happens implicitly when saving since the GimpObject name
           * of a GimpImage is the last-save URI
           */
          gimp_image_set_exported_file (image, file);
          gimp_image_set_export_proc (image, file_proc);

          /* An image can not be considered both exported and imported
           * at the same time, so stop consider it as imported now
           * that we consider it exported.
           */
          gimp_image_set_imported_file (image, NULL);

          gimp_image_export_clean_all (image);
        }

      if (export_backward || export_forward)
        gimp_image_exported (image, file);
      else
        gimp_image_saved (image, file);

      documents = GIMP_DOCUMENT_LIST (image->gimp->documents);

      imagefile = gimp_document_list_add_file (documents, file,
                                               file_proc->mime_type);

      /* only save a thumbnail if we are saving as XCF, see bug #25272 */
      if (GIMP_PROCEDURE (file_proc)->proc_type == GIMP_INTERNAL)
        gimp_imagefile_save_thumbnail (imagefile, file_proc->mime_type, image,
                                       NULL);
    }
  else if (status != GIMP_PDB_CANCEL)
    {
      if (error && *error == NULL)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("%s plug-in could not save image"),
                       gimp_plug_in_procedure_get_label (file_proc));
        }
    }

  gimp_image_flush (image);

 out:
  g_object_unref (file);
  g_object_unref (image);

  g_free (path);
  g_free (uri);
  if (export_image)
    g_object_unref (export_image);

  return status;
}

/*  private functions  */

/*
 * image_duplicate_scale:
 *
 * Returns: NULL if the export dimension are the same as the original,
 * or a duplicate #GimpImage scaled to the new dimensions otherwise.
 * The duplicate must be freed with g_object_unref(). */
static GimpImage *
image_duplicate_for_export (GimpImage    *image,
                            GimpProgress *progress)
{
  GimpImage             *new_image = NULL;
  /* Current resolution. */
  gdouble                current_xres;
  gdouble                current_yres;
  /* All export dimensions. */
  gint                   width;
  gint                   height;
  GimpUnit               unit;
  gdouble                xresolution;
  gdouble                yresolution;
  GimpUnit               resolution_unit;
  GimpInterpolationType  interpolation;

  /* Get current resolution. */
  gimp_image_get_resolution (image, &current_xres, &current_yres);

  /* Get export dimensions. */
  gimp_image_get_export_dimensions (image,
                                    &width,
                                    &height,
                                    &unit,
                                    &xresolution,
                                    &yresolution,
                                    &resolution_unit,
                                    &interpolation);
  if (width > 0 && height > 0)
    {
      if (width           != gimp_image_get_width  (image) ||
          height          != gimp_image_get_height (image) ||
          xresolution     != current_xres                  ||
          yresolution     != current_yres                  ||
          resolution_unit != gimp_image_get_unit (image))
        {
          new_image = gimp_image_duplicate (image);

          gimp_image_set_resolution (new_image, xresolution, yresolution);
          gimp_image_set_unit (new_image, resolution_unit);

          if (width  != gimp_image_get_width  (image) ||
              height != gimp_image_get_height (image))
            {
              gimp_image_scale (new_image, width, height, interpolation, progress);
            }
        }
    }
  else
    {
      g_warning ("Scale Error: "
                 "Both width and height must be greater than zero.");
    }

  return new_image;
}
