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
 * @file        symbols.c
 * @copyright   (c) 2011 - Jean-David Gadina - www.xs-labs.com
 * @abstract    Symbols functions
 */

/* Local includes */
#include "egz.h"

static unsigned int __percent = 0;

/*!
 * 
 */
egz_table * egz_create_table( void )
{
    unsigned int i;
    egz_table * table;
    
    /* Allocates memory for the table */
    if( NULL == ( table = ( egz_table * )malloc( sizeof( egz_table ) ) ) )
    {
        return NULL;
    }
    
    /* Initializes the table fields */
    table->count       = 0;
    table->total       = 0;
    table->information = 0;
    table->entropy     = 0;
    
    /* Initializes each character fields */
    for( i = 0; i < 256; i++ )
    {
        table->symbols[ i ].character  = ( unsigned char )i;
        table->symbols[ i ].occurences = 0;
        table->symbols[ i ].id         = 0;
        table->symbols[ i ].entropy    = 0;
        table->symbols[ i ].parent     = NULL;
        table->symbols[ i ].left       = NULL;
        table->symbols[ i ].right      = NULL;
    }
    
    return table;
}

/*!
 * 
 */
void egz_get_symbols( egz_table * table, FILE * source )
{
    unsigned int        i;
    unsigned int        j;
    unsigned char       buffer[ EGZ_READ_BUFFER_LENGTH ];
    size_t              length;
    long                offset;
    unsigned long       size;
    unsigned long       read_ops;
    unsigned long       read_op;
    libprogressbar_args args;
    
    __percent = 0;
    size      = egz_getfilesize( source );
    read_ops  = ceil( ( double )size / ( double )EGZ_READ_BUFFER_LENGTH );
    read_op   = 0;
    
    if( libdebug_is_enabled() == false )
    {
        args.percent = &__percent;
        args.length  = 50;
        args.label   = "Analyzing source file: ";
        args.done    = "[OK]";
        
        libprogressbar_create_progressbar( ( void * )( &args ) );
    }
    
    offset = ftell( source );
    
    fseek( source, 0, SEEK_SET );
    
    /* Reads the source file */
    while( ( length = fread( buffer, sizeof( char ), EGZ_READ_BUFFER_LENGTH, source ) ) > 0 )
    {
        read_op++;
        
        /* Process each byte read */
        for( i = 0; i < length; i++ )
        {
            /* Character ASCII code */
            j = buffer[ i ];
            
            /* Checks if the occurences of the symbol were already found */
            if( table->symbols[ j ].occurences == 0 )
            {
                /* No - Increase the total number of symbols in the table */
                table->count++;
            }
            
            /* Increases the number of ocurrences */
            table->symbols[ j ].occurences++;
            table->total++;
        }
        
        if( read_ops > 1 )
        {
            __percent = ( ( double )read_op / ( double )read_ops ) * 100;
        }
    }
    
    __percent = 100;
    
    libprogressbar_end();
    
    /* Process each symbol of the table */
    for( i = 0; i < 256; i++ )
    {
        /* Checks if the character was present in the source file */
        if( table->symbols[ i ].occurences > 0 )
        {
            /* Computes the character frequency */
            table->symbols[ i ].frequency = ( double )table->symbols[ i ].occurences / ( double )table->total;
            
            /* Computes the character information content */
            table->symbols[ i ].information = -LOG2( table->symbols[ i ].frequency );
            
            /* Computes the character entropy */
            table->symbols[ i ].entropy = table->symbols[ i ].frequency * LOG2( ( double )1 / table->symbols[ i ].frequency );
            
            /* Updates the table information content and entropy */
            table->information += table->symbols[ i ].information * table->symbols[ i ].occurences;
            table->entropy     += table->symbols[ i ].entropy * table->symbols[ i ].occurences;
        }
    }
    
    fseek( source, 0, SEEK_SET );
}

/*!
 * 
 */
void egz_sort_symbols_by_occurences( egz_symbol ** symbols, int left, int right )
{
    int          i;
    int          j;
    egz_symbol * x;
    egz_symbol * y;
    
    i = left;
    j = right;
    
    /* Gets the pivot value */
    x = symbols[ ( left + right ) / 2 ];
    
    /* End of the process */
    if( i > j )
    {
        return;
    }
    
    /* Process values */
    do
    {
        /* Leave smaller right values */
        while( ( symbols[ i ]->occurences < x->occurences ) && ( i < right ) )
        {
           i++;
        }
        
        /* Leave bigger left values */
        while( ( x->occurences < symbols[ j ]->occurences ) && ( j > left ) )
        {
            j--;
        }
        
        if( i <= j )
        {
            /* Swap values */
            y            = symbols[ i ];
            symbols[ i ] = symbols[ j ];
            symbols[ j ] = y;
            
            i++;
            j--;
        }
    }
    while( i <= j );
    
    /* Process remaining right values */
    if( i < right )
    {
        egz_sort_symbols_by_occurences( symbols, i, right );
    }
    
    /* Process remaining left values */
    if( left < j )
    {
        egz_sort_symbols_by_occurences( symbols, left, j );
    }
}

/*!
 * 
 */
void egz_sort_symbols_by_bits( egz_symbol ** symbols, int left, int right )
{
    int          i;
    int          j;
    egz_symbol * x;
    egz_symbol * y;
    
    i = left;
    j = right;
    
    /* Gets the pivot value */
    x = symbols[ ( left + right ) / 2 ];
    
    /* End of the process */
    if( i > j )
    {
        return;
    }
    
    /* Process values */
    do
    {
        /* Leave smaller right values */
        while( ( symbols[ i ]->bits < x->bits ) && ( i < right ) )
        {
           i++;
        }
        
        /* Leave bigger left values */
        while( ( x->bits < symbols[ j ]->bits ) && ( j > left ) )
        {
            j--;
        }
        
        if( i <= j )
        {
            /* Swap values */
            y            = symbols[ i ];
            symbols[ i ] = symbols[ j ];
            symbols[ j ] = y;
            
            i++;
            j--;
        }
    }
    while( i <= j );
    
    /* Process remaining right values */
    if( i < right )
    {
        egz_sort_symbols_by_bits( symbols, i, right );
    }
    
    /* Process remaining left values */
    if( left < j )
    {
        egz_sort_symbols_by_bits( symbols, left, j );
    }
}
