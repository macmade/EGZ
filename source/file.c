/*******************************************************************************
 * Copyright (c) 2010, Jean-David Gadina - www.xs-labs.com
 * Distributed under the Boost Software License, Version 1.0.
 * 
 * Boost Software License - Version 1.0 - August 17th, 2003
 * 
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

/* $Id$ */

/*!
 * @file        file.c
 * @copyright   (c) 2011 - Jean-David Gadina - www.xs-labs.com
 * @abstract    File functions
 */

/* Local includes */
#include "egz.h"

/*!
 * 
 */
bool egz_get_destination_filename( char * source, char * filename, bool compress )
{
    int    i;
    size_t length;
    size_t length_ext;
    char * basename;
    bool   add_ext;
    char   suffix[ 10 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    
    basename   = source;
    length     = strlen( basename );
    length_ext = strlen( EGZ_FILE_EXT );
    
    /* Process the string from the end */
    for( i = length - 1; i > -1; i-- )
    {
        /* Checks for a slash */
        if( basename[ i ] == '/' )
        {
            /* Moves the pointer to the file name */
            basename += i + 1;
            break;
        }
    }
    
    /* Gets the string length */
    length = strlen( basename );
    
    /* Resets the buffer */
    memset( filename, 0, FILENAME_MAX );
    
    /* Checks if we need to remove or add the file extension */
    if( compress == true )
    {
        /* Copies the file name and add the extension */
        add_ext = true;
        strcpy( filename, basename );
        strcat( filename, EGZ_FILE_EXT );
    }
    else if( strcmp( basename + length - length_ext, EGZ_FILE_EXT ) == 0 )
    {
        /* Copies the file name, without the extension */
        add_ext = false;
        strncpy( filename, basename, length - length_ext );
    }
    else
    {
        /* Copies the file name, without the extension */
        add_ext = false;
        strncpy( filename, basename, length );
    }
    
    i = 0;
    
    /* Checks if we need to add a suffix (if a file with the same name already exist) */
    while( access( filename, F_OK ) == 0 && i < 999999999 )
    {
        if( i > 0 )
        {
            memset( filename + ( strlen( filename ) - strlen( suffix ) ), 0, strlen( suffix ) );
        }
        
        /* Suffix to add */
        sprintf( suffix, "-%i", ++i );
        
        /* Checks if we must add the extension */
        if( add_ext == false )
        {
            /* Not enough space for the filename */
            if( strlen( suffix ) + length - length_ext > FILENAME_MAX )
            {
                break;
            }
            
            /* Adds the suffix */
            strncpy( filename, basename, length - length_ext );
            strcat( filename, suffix );
        }
        else
        {
            /* Not enough space for the filename */
            if( strlen( suffix ) + length + length_ext > FILENAME_MAX )
            {
                break;
            }
            
            /* Adds the suffix */
            strcpy( filename, basename );
            strcat( filename, suffix );
            strcat( filename, EGZ_FILE_EXT );
        }
    }
    
    /* Returns true if a destination filename was successfully created */
    return ( access( filename, F_OK ) == -1 ) ? true : false;
}

/*!
 * 
 */
unsigned long egz_getfilesize( FILE * fp )
{
    unsigned long offset;
    unsigned long size;
    
    offset = ftell( fp );
    
    fseek( fp, 0, SEEK_SET );
    fseek( fp, 0, SEEK_END );
    
    size = ftell( fp );
    
    fseek( fp, offset, SEEK_SET );
    
    return size;
}

/*!
 * 
 */
double egz_getfilesize_human( FILE * fp, char * unit )
{
    unsigned long offset;
    unsigned long bytes;
    double        size;
    
    memset( unit, 0, 3 );
    
    offset = ftell( fp );
    
    fseek( fp, 0, SEEK_END );
    
    bytes = ftell( fp );
    
    fseek( fp, offset, SEEK_SET );
    
    /* Checks the size range */
    if( bytes < 1000000 )
    {
        /* Kilo-Bytes */
        size   = ( double )bytes / ( double )1000;
        
        strcpy( unit, "KB" );
    }
    else if( bytes < 1000000000 )
    {
        /* Mega-Bytes */
        size = ( ( double )bytes / ( double )1000 ) / ( double )1000;
        
        strcpy( unit, "MB" );
    }
    else
    {
        /* Giga-Bytes */
        size = ( ( ( double )bytes / ( double )1000 ) / ( double )1000 ) / ( double )1000;
        
        strcpy( unit, "GB" );
    }
    
    return size;
}
