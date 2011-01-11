/*******************************************************************************
 * Copyright (c) 2010, Jean-David Gadina <macmade@eosgarden.com>
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
 * @file        compress.c
 * @copyright   eosgarden 2011 - Jean-David Gadina <macmade@eosgarden.com>
 * @abstract    Compression functions
 */

/* Local includes */
#include "egz.h"

/* Private variables */
static unsigned int __percent = 0;

/*!
 * 
 */
egz_status egz_compress( FILE * source, FILE * destination, bool force )
{
    unsigned int  i;
    unsigned int  j;
    double        size_original;
    double        size_compressed;
    double        ratio;
    char          unit_original[ 3 ];
    char          unit_compressed[ 3 ];
    egz_table  *  table;
    egz_symbol *  tree;
    egz_symbol ** symbols;
    egz_status    status;
    char              md5[ MD5_DIGEST_LENGTH * 2 + 1 ];
    
    memset( md5, 0, MD5_DIGEST_LENGTH * 2 + 1 );
    
    /* Creates the symbol table */
    DEBUG( "Creating the symbols table" );
    table = egz_create_table();
    
    /* Error - The table was not created */
    if( table == NULL )
    {
        return EGZ_ERROR_MALLOC;
    }
    
    /* Gets the symbols from the source file */
    DEBUG( "Getting all symbols from the source file" );
    egz_get_symbols( table, source );
    
    /* No symbols - Why compress an empty file? */
    if( table->count == 0 )
    {
        return EGZ_ERROR_EMPTY_FILE;
    }
    
    /* Prints the symbols table if debug is activated */
    if( libdebug_is_enabled() == true )
    {
        DEBUG( "Table of symbols:" );
        egz_print_table( table );
    }
    
    /* Allocates memory to store pointers to the symbols */
    if( NULL == ( symbols = ( egz_symbol ** )malloc( sizeof( egz_symbol * ) * table->count ) ) )
    {
        free( table );
        return EGZ_ERROR_MALLOC;
    }
    
    j = 0;
    
    /* Process each ASCII character */
    for( i = 0; i < 256; i++ )
    {
        /* Checks if the character was present in the file */
        if( table->symbols[ i ].occurences > 0 )
        {
            /* Stores a pointer to the symbol structure */
            symbols[ j++ ] = &( table->symbols[ i ] );
        }
    }
    
    /* Sorts the symbols by their frequency */
    DEBUG( "Ordering the symbols by their frequency" );
    egz_sort_symbols_by_occurences( symbols, 0, table->count - 1 );
    
    /* Prints the list of the ordered symbols */
    if( libdebug_is_enabled() == true )
    {
        DEBUG( "Ordered symbols:" );
        egz_print_symbols( symbols, table->count );
    }
    
    DEBUG( "Creating the binary tree of symbols" );
    
    /* No need for a tree if there is only one symbol */
    if( table->count > 1 )
    {
        /* Creates the binary tree (usual algorithm) */
        tree = egz_create_btree( symbols, table->count );
        
        /* Error - The tree was not created */
        if( tree == NULL )
        {
            free( symbols );
            free( table );
            return EGZ_ERROR_MALLOC;
        }
    }
    else
    {
        /* One symbol, so the tree is that symbol */
        tree = *( symbols );
    }
    
    /* Creates the binary codes for each symbol of the tree */
    DEBUG( "Determining symbol codes" );
    egz_create_codes( tree, 0, 0 );
    
    /* Prints the symbol binary codes */
    if( libdebug_is_enabled() == true )
    {
        DEBUG( "Symbol codes:" );
        egz_print_codes( symbols, table->count );
        DEBUG( "Statistics:" );
        egz_print_statistics( symbols, table->count );
    }
    
    if( force == false )
    {
        DEBUG( "Checking final compression ratio" );
        status = egz_check_compression_ratio( table );
        
        if( status != EGZ_OK )
        {
            DEBUG( "Freeing memory" );
            
            if( table->count > 1 )
            {
                free( tree );
            }
            
            free( table );
            free( symbols );
            
            return status;
        }
    }
    
    DEBUG( "Writing file header" );
    egz_write_header( source, destination, table );
    
    /* Prints the header as hexadecimal */
    if( libdebug_is_enabled() == true )
    {
        DEBUG( "Header:" );
        egz_print_file_ptr( destination );
    }
    
    DEBUG( "Compressing file" );
    egz_write_compressed_file( source, destination, table );
    egz_file_md5_checksum( source, md5 );
    
    size_original   = egz_getfilesize_human( source, unit_original );
    size_compressed = egz_getfilesize_human( destination, unit_compressed );
    ratio           = egz_get_compression_ratio( table );
    
    printf
    (
        "Original file size:     %.2f %s\n"
        "Compressed file size:   %.2f %s\n"
        "Compression ratio:      %.2f %%\n"
        "Original file checksum: %s\n",
        size_original,
        unit_original,
        size_compressed,
        unit_compressed,
        ratio,
        md5
    );
    DEBUG( "Freeing memory" );
    
    if( table->count > 1 )
    {
        free( tree );
    }
    
    free( table );
    free( symbols );
    
    return EGZ_OK;
}

/*!
 *
 */
uint16_t egz_get_header_size( egz_table * table )
{
    static uint16_t size = 0;
    unsigned int    i;
    
    if( size > 0 )
    {
        return size;
    }
    
    /* Number of symbols + original file size + MD5 checksum */
    size = 43;
    
    for( i = 0; i < 256; i++ )
    {
        if( table->symbols[ i ].bits > 0 )
        {
            size += 2;
            
            if( table->symbols[ i ].bits > 32 )
            {
                size += 8;
            }
            else if( table->symbols[ i ].bits > 16 )
            {
                size += 4;
            }
            else if( table->symbols[ i ].bits > 8 )
            {
                size += 2;
            }
            else
            {
                size += 1;
            }
        }
    }
    
    return size;
}

/*!
 * 
 */
egz_status egz_check_compression_ratio( egz_table * table )
{
    unsigned char c;
    char          answer[ 1 ];
    unsigned long bits_original;
    unsigned long bits_compressed;
    unsigned long bytes_original;
    unsigned long bytes_compressed;
    egz_symbol  * symbol;
    
    bits_original   = 0;
    bits_compressed = 0;
    
    /* Process each symbol */
    for( c = 0; c < 255; c++ )
    {
        /* Current symbol */
        symbol = &( table->symbols[ c ] );
        
        if( symbol->bits > 0 )
        {
            /* Adds the number of bits used to represent the symbol in the original file */
            bits_original   += 8 * symbol->occurences;
            
            /* Adds the number of bits used to represent the symbol in the compressed file */
            bits_compressed += symbol->bits * symbol->occurences;
        }
    }
    
    bytes_original    = bits_original   / 8;
    bytes_compressed  = bits_compressed / 8;
    bytes_compressed += egz_get_header_size( table );
    
    DEBUG( "Original file:                 %lu bytes", bytes_original );
    DEBUG( "Compressed file (with header): %lu bytes", bytes_compressed );
    
    if( bytes_original < bytes_compressed )
    {
        printf
        (
            "Warning: the compressed file will be larger than the original file.\n"
            "Do you still want to continue? [y/N]\n"
        );
        
        scanf( "%c", answer );
        
        if( *( answer ) != 'y' && *( answer ) != 'Y' )
        {
            return EGZ_ERROR_ABORT;
        }
    }
    
    return EGZ_OK;
}

/*!
 * 
 */
double egz_get_compression_ratio( egz_table * table )
{
    unsigned int  i;
    unsigned long bits_original;
    unsigned long bits_compressed;
    unsigned long bytes_original;
    unsigned long bytes_compressed;
    double        ratio;
    egz_symbol  * symbol;
    
    bits_original   = 0;
    bits_compressed = 0;
    
    /* Process each symbol */
    for( i = 0; i < 256; i++ )
    {
        /* Current symbol */
        symbol = &( table->symbols[ i ] );
        
        if( symbol->bits > 0 )
        {
            /* Adds the number of bits used to represent the symbol in the original file */
            bits_original   += 8 * symbol->occurences;
            
            /* Adds the number of bits used to represent the symbol in the compressed file */
            bits_compressed += symbol->bits * symbol->occurences;
        }
    }
    
    bytes_original    = bits_original   / 8;
    bytes_compressed  = bits_compressed / 8;
    bytes_compressed += egz_get_header_size( table );
    
    ratio = 100 - ( ( double )bytes_compressed / ( double )bytes_original ) * 100;
    
    return ratio;
}

egz_status egz_write_header( FILE * source, FILE * destination, egz_table * table )
{
    unsigned int    i;
    uint16_t        header_size;
    uint64_t        file_size;
    char          * md5;
    unsigned char * header;
    
    header_size = egz_get_header_size( table );
    file_size   = egz_getfilesize( source );
    
    if( NULL == ( md5 = ( char * )calloc( sizeof( char ), ( MD5_DIGEST_LENGTH * 2 + 1 ) ) ) )
    {
        return EGZ_ERROR_MALLOC;
    }
    
    if( NULL == ( header = ( unsigned char * )calloc( sizeof( unsigned char ), header_size + 5 ) ) )
    {
        return EGZ_ERROR_MALLOC;
    }

    fwrite( EGZ_FILE_ID,        sizeof( uint8_t ),  strlen( EGZ_FILE_ID ),        destination );
    fwrite( &header_size,       sizeof( uint16_t ), 1,                            destination );
    fwrite( EGZ_FILE_HEADER_ID, sizeof( uint8_t ),  strlen( EGZ_FILE_HEADER_ID ), destination );
    fwrite( &file_size,         sizeof( uint64_t ), 1,                            destination );
    
    DEBUG( "Getting source file MD5 checksum" );
    egz_file_md5_checksum( source, md5 );
    DEBUG( "MD5 checksum: %s", md5 );
    fwrite( md5, sizeof( char ), MD5_DIGEST_LENGTH * 2 + 1, destination );
    free( md5 );
    
    fwrite( &( table->count ), sizeof( unsigned short ), 1, destination );
    
    for( i = 0; i < 256; i++ )
    {
        if( table->symbols[ i ].bits > 0 )
        {
            fwrite( &( table->symbols[ i ].character ), sizeof( unsigned char ), 1, destination );
            fwrite( &( table->symbols[ i ].bits      ), sizeof( unsigned char ), 1, destination );
            
            if( table->symbols[ i ].bits > 32 )
            {
                fwrite( &( table->symbols[ i ].code ), sizeof( uint64_t ), 1, destination );
            }
            else if( table->symbols[ i ].bits > 16 )
            {
                fwrite( &( table->symbols[ i ].code ), sizeof( uint32_t ), 1, destination );
            }
            else if( table->symbols[ i ].bits > 8 )
            {
                fwrite( &( table->symbols[ i ].code ), sizeof( uint16_t ), 1, destination );
            }
            else
            {
                fwrite( &( table->symbols[ i ].code ), sizeof( uint8_t ), 1, destination );
            }
        }
    }
    
    free( header );
    
    return EGZ_OK;
}

/*!
 * 
 */
egz_status egz_write_compressed_file( FILE * source, FILE * destination, egz_table * table )
{
    unsigned int        num_bits;
    unsigned int        length;
    unsigned int        i;
    unsigned int        j;
    unsigned long       size;
    unsigned long       read_ops;
    unsigned long       read_op;
    long                offset;
    unsigned char       c;
    unsigned char       read_buffer[ EGZ_READ_BUFFER_LENGTH ];
    unsigned char       symbol_buffer[ sizeof( uint64_t ) ];
    uint64_t            write_buffer[ EGZ_WRITE_BUFFER_LENGTH ];
    uint64_t          * data;
    egz_symbol        * s;
    libprogressbar_args args;
    
    j         = 0;
    num_bits  = 0;
    data      = ( uint64_t * )( &( symbol_buffer[ 0 ] ) );
    s         = NULL;
    offset    = ftell( source );
    size      = egz_getfilesize( source );
    read_ops  = ceil( ( double )size / ( double )EGZ_READ_BUFFER_LENGTH );
    read_op   = 0;
    __percent = 0;
    
    if( libdebug_is_enabled() == false )
    {
        args.percent = &__percent;
        args.length  = 50;
        args.label   = "Compressing file:      ";
        args.done    = "[OK]";
        
        libprogressbar_create_progressbar( ( void * )( &args ) );
    }
    
    fseek( source, 0, SEEK_SET );
    memset( read_buffer, 0, EGZ_READ_BUFFER_LENGTH );
    memset( symbol_buffer, 0, sizeof( uint64_t ) );
    memset( write_buffer, 0, EGZ_WRITE_BUFFER_LENGTH );
    fwrite( EGZ_FILE_DATA_ID, sizeof( uint8_t ), strlen( EGZ_FILE_DATA_ID ), destination );

    
    while( ( length = fread( read_buffer, sizeof( unsigned char ), EGZ_READ_BUFFER_LENGTH, source ) ) )
    {
        DEBUG( "Getting data from source file" );
        
        i        = 0;
        read_op += 1;
        
        write_compressed_file_process_symbols:
        
        for( ; i < length; i++ )
        {
            if( num_bits >= EGZ_BTREE_CODE_MAX_LENGTH )
            {
                break;
            }
            
            c = read_buffer[ i ];
            s = &( table->symbols[ ( int )c ] );
            
            if( EGZ_BTREE_CODE_MAX_LENGTH - num_bits > s->bits  )
            {
                *( data ) |= s->code << ( ( EGZ_BTREE_CODE_MAX_LENGTH - num_bits ) - s->bits );
            }
            else
            {
                *( data ) |= s->code >> ( s->bits - ( EGZ_BTREE_CODE_MAX_LENGTH - num_bits ) );
            }
            
            DEBUG
            (
                "    - Processing symbol 0x%02X (%c) | Bits: %02u | Left shift: %03i | Buffer: %lu",
                ( unsigned int )c,
                ( isprint( c ) && c != 0x20 ) ? c : '.',
                s->bits,
                ( EGZ_BTREE_CODE_MAX_LENGTH - num_bits ) - s->bits,
                *( data )
             );
            
            num_bits += s->bits;
        }
        
        if( num_bits >= EGZ_BTREE_CODE_MAX_LENGTH )
        {
            num_bits          = num_bits - EGZ_BTREE_CODE_MAX_LENGTH;
            write_buffer[ j ] = *( data );
            
            j++;
            
            DEBUG
            (
                "Storing data (%lu) to the buffer %i (bits left: %u)",
                *( data ),
                j,
                num_bits
            );
            
            if( j == EGZ_WRITE_BUFFER_LENGTH / ( EGZ_BTREE_CODE_MAX_LENGTH / 8 ) )
            {
                DEBUG( "Writing data to the destination file" );
                fwrite( write_buffer, sizeof( uint64_t ), j + 1, destination );
                memset( write_buffer, 0, EGZ_WRITE_BUFFER_LENGTH );
                
                j = 0;
            }
            
            if( num_bits > 0 )
            {
                *( data ) = s->code << ( EGZ_BTREE_CODE_MAX_LENGTH - num_bits );
                DEBUG( "Symbol buffer value: %lu (%u bits)", *( data ), num_bits );
            }
            else
            {
                *( data ) = 0;
            }
            
        }
        
        if( i < length )
        {
            goto write_compressed_file_process_symbols;
        }
        
        if( read_ops > 1 )
        {
            __percent = ( ( double )read_op / ( double )read_ops ) * 100;
        }
    }
    
    if( num_bits > 0 || j > 0 )
    {
        DEBUG( "Writing remaining data to the destination file" );
        
        if( num_bits < EGZ_BTREE_CODE_MAX_LENGTH )
        {
            write_buffer[ j ] = *( data );
        }
        fwrite( write_buffer, sizeof( uint64_t ), j + 1, destination );
    }
    
    __percent = 100;
    
    libprogressbar_end();
    
    fseek( source, offset, SEEK_SET );
    
    return EGZ_OK;
}
