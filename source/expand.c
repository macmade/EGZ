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
 * @file        expand.c
 * @copyright   eosgarden 2011 - Jean-David Gadina <macmade@eosgarden.com>
 * @abstract    Expansion functions
 */

/* Local includes */
#include "egz.h"

/* Private variables */
static unsigned int __percent = 0;

egz_status egz_expand( FILE * source, FILE * destination )
{
    long            offset;
    unsigned int    count;
    uint16_t        header_length;
    uint64_t        bytes;
    unsigned char * header;
    unsigned char * md5;
    egz_symbol    * symbols;
    egz_symbol    * tree;
    egz_status      status;
    char            id[ 4 ]        = { 0, 0, 0, 0 };
    char            header_id[ 4 ] = { 0, 0, 0, 0 };
    
    offset = ftell( source );
    
    fseek( source, 0, SEEK_SET );
    
    DEBUG( "Verifying the file signature" );
    if( fread( id, sizeof( uint8_t ), 3, source ) != 3 || strcmp( id, EGZ_FILE_ID ) != 0 )
    {
        return EGZ_ERROR_INVALID_FORMAT;
    }
    
    DEBUG( "Getting the header's length" );
    if( ( fread( &header_length, sizeof( uint16_t ), 1, source ) != 1 ) )
    {
        return EGZ_ERROR_INVALID_FORMAT;
    }
    
    if( NULL == ( header = ( unsigned char * )malloc( header_length ) ) )
    {
        return EGZ_ERROR_MALLOC;
    }
    
    if( fread( header_id, sizeof( uint8_t ), 3, source ) != 3 || strcmp( header_id, EGZ_FILE_HEADER_ID ) != 0 )
    {
        return EGZ_ERROR_INVALID_FORMAT;
    }
    
    DEBUG( "Getting the header data (%hu bytes)", header_length );
    
    if( fread( header, sizeof( uint8_t ), header_length, source ) != header_length )
    {
        free( header );
        return EGZ_ERROR_INVALID_FORMAT;
    }
    
    md5   = ( header + sizeof( uint64_t ) );
    bytes = ( ( uint64_t )( *( header + 7 ) ) << 56 )
    | ( ( uint64_t )( *( header + 6 ) ) << 48 )
    | ( ( uint64_t )( *( header + 5 ) ) << 40 )
    | ( ( uint64_t )( *( header + 4 ) ) << 32 )
    | ( ( uint64_t )( *( header + 3 ) ) << 24 )
    | ( ( uint64_t )( *( header + 2 ) ) << 16 )
    | ( ( uint64_t )( *( header + 1 ) ) << 8 )
    |   ( uint64_t )( *( header ) );
    
    DEBUG( "Original file is %lu bytes", bytes );
    DEBUG( "Original file MD5 checksum: %s", md5 );
    DEBUG( "Getting symbols informations" );
    
    count = egz_rebuild_symbols( header + 41, &symbols, header_length - 41 );
    
    DEBUG( "Rebuilding the binray tree of symbols" );
    
    if( count == 0 )
    {
        free( header );
        return EGZ_ERROR_INVALID_FORMAT;
    }
    else if( count == 0xFFFFFFFF )
    {
        free( header );
        
        return EGZ_ERROR_MALLOC;
    }
    
    status = egz_rebuild_tree( &tree, symbols, count );
    
    if( status != EGZ_OK )
    {
        free( symbols );
        free( header );
        
        return status;
    }
    
    DEBUG( "Expanding file" );
    
    status = egz_write_expanded_file( source, destination, tree, bytes );
    
    if( status != EGZ_OK )
    {
        free( tree );
        free( header );
        free( symbols );
        
        return status;
    }
    
    status = egz_verify_checksum( destination, md5 );
    
    if( status != EGZ_OK )
    {
        free( tree );
        free( header );
        free( symbols );
        
        return status;
    }
    
    free( tree );
    free( symbols );
    free( header );
    fseek( source, offset, SEEK_SET );
    
    return EGZ_OK;
}

unsigned int egz_rebuild_symbols( unsigned char * data, egz_symbol ** symbols_ptr, uint16_t length )
{
    unsigned char c;
    unsigned int  i;
    unsigned int  count;
    egz_symbol *  s;
    egz_symbol *  symbols;
    egz_symbol ** symbols_debug;
    
    i             = 0;
    count         = ( ( uint8_t )( *( data + 1 ) ) << 8 ) | ( uint8_t )( *( data ) );
    data         += 2;
    symbols_debug = NULL;
    
    if( NULL == ( symbols = ( egz_symbol * )malloc( sizeof( egz_symbol ) * count ) ) )
    {
        return 0xFFFFFFFF;
    }
    
    *( symbols_ptr ) = symbols;
    
    DEBUG( "%u symbols are present in the file", count );
    DEBUG( "Getting information about each symbol" );
    
    while( i < length )
    {
        c             = *( data++ );
        s             = symbols;
        
        symbols++;
        
        s->character   = c;
        s->id          = 0;
        s->bits        = ( *( data++ ) );
        s->code        = ( uint64_t )0;
        s->occurences  = 0;
        s->frequency   = 0;
        s->information = 0;
        s->entropy     = 0;
        s->parent      = NULL;
        s->left        = NULL;
        s->right       = NULL;
        i             += 2;
        
        if( s->bits > 32 )
        {
            s->code = ( ( uint64_t )( *( data + 7 ) ) << 56 )
                    | ( ( uint64_t )( *( data + 6 ) ) << 48 )
                    | ( ( uint64_t )( *( data + 5 ) ) << 40 )
                    | ( ( uint64_t )( *( data + 4 ) ) << 32 )
                    | ( ( uint64_t )( *( data + 3 ) ) << 24 )
                    | ( ( uint64_t )( *( data + 2 ) ) << 16 )
                    | ( ( uint64_t )( *( data + 1 ) ) << 8 )
                    |   ( uint64_t )( *( data ) );
            data   += 8;
            i      += 8;
        }
        else if( s->bits > 16 )
        {
            s->code = ( ( uint64_t )( *( data + 3 ) ) << 24 )
                    | ( ( uint64_t )( *( data + 2 ) ) << 16 )
                    | ( ( uint64_t )( *( data + 1 ) ) << 8 )
                    |   ( uint64_t )( *( data ) );
            data   += 4;
            i      += 4;
        }
        else if( s->bits > 8 )
        {
            s->code = ( ( uint64_t )( *( data + 1 ) ) << 8 )
                    | ( uint64_t )( *( data ) );
            data   += 2;
            i      += 2;
        }
        else
        {
            s->code = ( uint64_t )( *( data ) );
            data   += 1;
            i      += 1;
        }
    }
    
    symbols = *( symbols_ptr );
    
    if( libdebug_is_enabled() == true && NULL != ( symbols_debug = ( egz_symbol ** )malloc( sizeof( egz_symbol * ) * count ) ) )
    {
        for( i = 0; i < count; i++ )
        {
            symbols_debug[ i ] = &( symbols[ i ] );
        }
        
        DEBUG( "Sorting symbols by the number of bits" );
        egz_sort_symbols_by_bits( symbols_debug, 0, count - 1 );
        
        DEBUG( "Symbol codes:" );
        egz_print_codes( symbols_debug, count );
        free( symbols_debug );
    }
    
    return count;
}

egz_status egz_rebuild_tree( egz_symbol ** tree_ptr, egz_symbol * symbols, unsigned int count )
{
    unsigned char code;
    uint64_t      mask;
    unsigned int  bits;
    unsigned int  i;
    unsigned int  j;
    int           k;
    unsigned int  bytes;
    unsigned int  node_id;
    egz_symbol  * tree;
    egz_symbol  * branch;
    egz_symbol  * node;
    egz_symbol  * s;
    
    node_id = 0;
    
    if( NULL == ( tree = ( egz_symbol * )malloc( sizeof( egz_symbol ) * ( count - 1 ) ) ) )
    {
        return EGZ_ERROR_MALLOC;
    }
    
    branch        = tree;
    node          = tree;
    *( tree_ptr ) = tree;
    i             = 0;
    j             = 0;
    k             = 0;
    
    for( i = 0; i < count - 1; i++ )
    {
        tree[ i ].character   = 0;
        tree[ i ].id          = ++node_id;
        tree[ i ].bits        = 0;
        tree[ i ].code        = 0;
        tree[ i ].occurences  = 0;
        tree[ i ].frequency   = 0;
        tree[ i ].information = 0;
        tree[ i ].entropy     = 0;
        tree[ i ].parent      = NULL;
        tree[ i ].left        = NULL;
        tree[ i ].right       = NULL;
    }
    DEBUG( "OK Count: %u", count );
    for( i = 0; i < count; i++ )
    {
        s = &( symbols[ i ] );
        
        if( s->bits > 32 )
        {
            bytes = 8;
        }
        else if( s->bits > 16 )
        {
            bytes = 4;
        }
        else if( s->bits > 8 )
        {
            bytes = 2;
        }
        else
        {
            bytes = 1;
        }
        
        for( j = 0; j < bytes; j++ )
        {
            mask = ( uint64_t )( 0xFF << ( ( j ) * 8 ) );
            code = ( unsigned char )( ( s->code & mask ) >> ( ( j ) * 8 ) );
            bits = ( bytes == 1 || ( j > 0 && j == bytes - 1 ) ) ? s->bits : 8;
            
            DEBUG( "Processing character 0x%02X (%c) - %u bits", s->character, ( isprint( s->character ) && s->character != 0x20 ) ? s->character : '.', s->bits );
            
            if( s->character == 255 )
            {
                DEBUG( "OKOKOK" );
            }
            
            for( k = bits - 1; k > -1; k-- )
            {
                if( code & ( 1 << k ) )
                {
                    if( k == 0 )
                    {
                        DEBUG( "Placing right symbol: 0x%02X (%c)", s->character, ( isprint( s->character ) && s->character != 0x20 ) ? s->character : '.' );
                        
                        branch->right = s;
                    }
                    else if( branch->right == NULL )
                    {
                        node++;
                        
                        DEBUG( "    - Creating new internal right node: #%u", node->id );
                        
                        branch->right = node;
                        branch        = branch->right;
                        
                    }
                    else
                    {
                        DEBUG( "    - Moving on right branch: #%u", branch->right->id );
                        branch = branch->right;
                    }
                }
                else
                {
                    if( k == 0 )
                    {
                        DEBUG( "Placing left symbol:  0x%02X (%c)", s->character, ( isprint( s->character ) && s->character != 0x20 ) ? s->character : '.' );
                        
                        branch->left = s;
                    }
                    else if( branch->left == NULL )
                    {
                        node++;
                        
                        DEBUG( "    - Creating new internal left node:  #%u", node->id );
                        
                        branch->left = node;
                        branch       = branch->left;
                    }
                    else
                    {
                        DEBUG( "    - Moving on left  branch: #%u", branch->left->id );
                        branch = branch->left;
                    }
                }
            }
        }
        
        branch = tree;
    }
    
    if( libdebug_is_enabled() == true )
    {
        DEBUG( "Tree nodes:" );
        
        for( i = 0; i < count - 1; i++ )
        {
            egz_print_node( &( tree[ i ] ) );
        }
    }
    
    return EGZ_OK;
}

/*!
 * 
 */
egz_status egz_write_expanded_file( FILE * source, FILE * destination, egz_symbol * tree, uint64_t filesize )
{
    unsigned int        i;
    unsigned int        j;
    uint16_t            header_length;
    unsigned int        length;
    unsigned long       size;
    unsigned long       read_ops;
    unsigned long       read_op;
    long                offset;
    uint64_t            bytes;
    uint64_t            bytes_total;
    uint64_t            c;
    uint64_t            read_buffer[ EGZ_READ_BUFFER_LENGTH ];
    unsigned char       write_buffer[ EGZ_WRITE_BUFFER_LENGTH ];
    libprogressbar_args args;
    egz_symbol        * branch;
    char                data_id[ 4 ] = { 0, 0, 0, 0 };
    
    offset      = ftell( source );
    size        = egz_getfilesize( source );
    read_ops    = ceil( ( double )size / ( double )EGZ_READ_BUFFER_LENGTH );
    read_op     = 0;
    bytes       = 0;
    bytes_total = 0;
    __percent   = 0;
    
    if( libdebug_is_enabled() == false )
    {
        args.percent = &__percent;
        args.length  = 50;
        args.label   = "Expanding file:        ";
        args.done    = "[OK]";
        
        libprogressbar_create_progressbar( ( void * )( &args ) );
    }
    
    fseek( source, strlen( EGZ_FILE_ID ), SEEK_SET );
    fread( &header_length, sizeof( uint16_t ), 1, source );
    fseek( source, header_length + strlen( EGZ_FILE_HEADER_ID ), SEEK_CUR );
    
    memset( read_buffer,  0, EGZ_READ_BUFFER_LENGTH );
    memset( write_buffer, 0, EGZ_WRITE_BUFFER_LENGTH );
    
    if( fread( data_id, sizeof( uint8_t ), 3, source ) != 3 || strcmp( data_id, EGZ_FILE_DATA_ID ) != 0 )
    {
        return EGZ_ERROR_INVALID_FORMAT;
    }
    
    branch = tree;
    
    while( ( length = fread( read_buffer, sizeof( uint64_t ), EGZ_READ_BUFFER_LENGTH, source ) ) )
    {
        read_ops++;
        
        for( i = 0; i < length; i++ )
        {
            c = read_buffer[ i ];
            
            for( j = 0; j < ( sizeof( uint64_t ) * 8 ); j++ )
            {
                if( bytes_total == filesize )
                {
                    break;
                }
                
                if( ( c >> ( ( ( sizeof( uint64_t ) * 8 ) - 1 ) - j ) ) & 1 )
                {
                    DEBUG( "        - Moving on right branch: #%u", branch->right->id );
                    branch = branch->right;
                }
                else
                {
                    DEBUG( "        - Moving on left  branch: #%u", branch->left->id );
                    branch = branch->left;
                }
                
                if( branch->bits > 0 )
                {
                    DEBUG( "    - Found character 0x%02X (%c) - %u bits", branch->character, ( isprint( branch->character ) && branch->character != 0x20 ) ? branch->character : '.', branch->bits );
                    write_buffer[ bytes ] = branch->character;
                    
                    bytes++;
                    bytes_total++;
                    
                    if( bytes == EGZ_WRITE_BUFFER_LENGTH )
                    {
                        DEBUG( "Writing data to the destination file" );
                        fwrite( write_buffer, sizeof( unsigned char ), EGZ_WRITE_BUFFER_LENGTH, destination );
                        memset( write_buffer, 0, EGZ_WRITE_BUFFER_LENGTH );
                        
                        bytes = 0;
                    }
                    
                    DEBUG( "    - Moving on tree top" );
                    branch = tree;
                    
                }
            }
            
            if( bytes_total == filesize )
            {
                break;
            }
        }
        
        if( read_ops > 1 )
        {
            __percent = ( ( double )read_op / ( double )read_ops ) * 100;
        }
    }
    
    if( bytes > 0 )
    {
        DEBUG( "Writing remaining data to the destination file" );
        fwrite( write_buffer, sizeof( unsigned char ), bytes, destination );
    }
    
    __percent = 100;
    
    libprogressbar_end();
    
    return EGZ_OK;
}

/*!
 * 
 */
egz_status egz_verify_checksum( FILE * destination, unsigned char * checksum )
{
    char answer[ 1 ];
    char md5[ MD5_DIGEST_LENGTH * 2 + 1 ];
    
    DEBUG( "Getting destination file MD5 checksum" );
    egz_file_md5_checksum( destination, md5 );
    DEBUG( "New MD5 checksum:      %s", md5 );
    DEBUG( "Original MD5 checksum: %s", checksum );
    DEBUG( "Verifiying MD5 checksum:" );
    
    if( strcmp( ( char * )checksum, md5 ) != 0 )
    {
        printf
        (
         "Warning: the original file MD5 checksum does not correspond to the expanded file.\n"
         "\n"
         "Original checksum: %s\n"
         "New checksum:      %s\n"
         "\n"
         "Do you still want to continue? [y/N]\n",
         checksum,
         md5
         );
        
        scanf( "%c", answer );
        
        if( *( answer ) != 'y' && *( answer ) != 'Y' )
        {
            return EGZ_ERROR_ABORT;
        }
    }
    else
    {
        DEBUG( "MD5 checksum successfully verified" );
    }
    
    return EGZ_OK;
}