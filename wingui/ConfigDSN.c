/* Copyright (C) 2000-2007 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   There are special exceptions to the terms and conditions of the GPL as it
   is applied to this software. View the full text of the exception in file
   EXCEPTIONS in the directory of this software distribution.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifdef _WIN32
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include "installer.h"
#include "stringutil.h"
#include "odbcdialogparams/odbcdialogparams.h"

/* TODO no L"" */
static SQLWCHAR *W_INVALID_ATTR_STR = L"Invalid attribute string";
static SQLWCHAR *W_USER_CANCELLED = L"User cancelled";

BOOL Driver_Prompt(HWND hWnd, SQLWCHAR *instr, SQLUSMALLINT completion,
                   SQLWCHAR *outstr, SQLINTEGER outmax, SQLINTEGER *outlen)
{
  DataSource *ds= ds_new();
  BOOL rc= FALSE;

  /*
     parse the attr string, dsn lookup will have already been
     done in the driver
  */
  if (instr && *instr)
  {
    if (ds_from_kvpair(ds, instr, (SQLWCHAR)';'))
    {
      SQLPostInstallerError(ODBC_ERROR_INVALID_KEYWORD_VALUE,
                            W_INVALID_ATTR_STR);
      goto exit;
    }
  }

  /* TODO make sure the ds->driver is ok, for Test,etc */
  /* Show the dialog and handle result */
  if (ShowOdbcParamsDialog(ds, hWnd, TRUE) == 1)
  {
    /* serialize to outstr */
    if ((*outlen= ds_to_kvpair(ds, outstr, outmax, (SQLWCHAR)';')) == -1)
    {
      /* truncated, up to caller to see outmax == *outlen */
      *outlen= outmax;
      outstr[outmax]= 0;
    }
    rc= TRUE;
  }
  else
  {
    SQLPostInstallerError(ODBC_ERROR_INVALID_KEYWORD_VALUE,
                          W_USER_CANCELLED);
  }

exit:
  ds_delete(ds);
  return rc;
}


/*
   Add, edit, or remove a Data Source Name (DSN). This function is
   called by "Data Source Administrator" on Windows, or similar
   application on Unix.
*/
BOOL INSTAPI ConfigDSNW(HWND hWnd, WORD nRequest, LPCWSTR pszDriver,
                        LPCWSTR pszAttributes)
{
  DataSource *ds= ds_new();
  BOOL rc= TRUE;
  SQLWCHAR *driverfile;

  if (pszAttributes && *pszAttributes)
  {
    if (ds_from_kvpair(ds, pszAttributes, (SQLWCHAR)';'))
    {
      SQLPostInstallerError(ODBC_ERROR_INVALID_KEYWORD_VALUE,
                            W_INVALID_ATTR_STR);
      rc= FALSE;
      goto exitConfigDSN;
    }
    if (ds_lookup(ds))
    {
      /* ds_lookup() will already set SQLInstallerError */
      rc= FALSE;
      goto exitConfigDSN;
    }
  }

  /*
     We set the actual name of the driver on the DataSource. This is
     needed when testing the connection. We save the file name (which
     was given to us, to save back to the registry).
  */
  driverfile= ds->driver;
  ds->driver= NULL;
  /* TODO .... constant for driver name */
	ds_set_strattr(&ds->driver, L"MySQL ODBC 5.1 Driver");

  switch (nRequest)
  {
  case ODBC_ADD_DSN:
  case ODBC_CONFIG_DSN:
    if (ShowOdbcParamsDialog(ds, hWnd, FALSE) == 1)
    {
      ds_set_strattr(&ds->driver, driverfile);
      /* save datasource */
      if (ds_add(ds))
        rc= FALSE;
    }
    break;
  case ODBC_REMOVE_DSN:
    if (SQLRemoveDSNFromIni(ds->name) != TRUE)
      rc= FALSE;
    break;
  }

exitConfigDSN:
  x_free(driverfile);
  ds_delete(ds);
  return rc;
}

