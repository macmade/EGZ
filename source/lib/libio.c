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
 * @file        libio.c
 * @copyright   eosgarden 2011 - Jean-David Gadina <macmade@eosgarden.com>
 * @abstract    IO functions
 */

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Definition of the boolean datatype if not enabled */
#ifndef __bool_true_false_are_defined
#define bool                            _Bool
#define true                            1
#define false                           0
#define __bool_true_false_are_defined   1
#endif

#define __LIBIO_INIT( f )           \
if( f->need_init == true )          \
{                                   \
if( f->stdin == true )          \
{                               \
f->fp        = stdin;       \
f->need_init = false;       \
}                               \
else if( f->stdout == true )    \
{                               \
f->fp        = stdout;      \
f->need_init = false;       \
}                               \
else if( f->stderr == true )    \
{                               \
f->fp        = stderr;      \
f->need_init = false;       \
}                               \
}

/*!
 * 
 */
struct libio_file
{
    FILE      * fp;
    char        filename[ FILENAME_MAX ];
    char        mode[ 4 ];
    bool        writeable;
    bool        readable;
    bool        stdin;
    bool        stdout;
    bool        stderr;
    bool        need_init;
    uint8_t     bit_buffer;
    uint8_t     bit_count;
    uint8_t     bit_offset;
    struct stat stat_buf;
};

/* Default I/O streams */
struct libio_file libio_stdin  = { .stdin  = true, .need_init = true };
struct libio_file libio_stdout = { .stdout = true, .need_init = true };
struct libio_file libio_stderr = { .stderr = true, .need_init = true };

/* Functions prototypes - stdio replacement */
struct libio_file * libio_fopen(    const char * filename, const char * mode );
struct libio_file * libio_freopen(  const char * filename, const char * mode, struct libio_file * stream );
int                 libio_fflush(   struct libio_file * stream );
int                 libio_fclose(   struct libio_file * stream );
int                 libio_fprintf(  struct libio_file * stream, const char * format, ... );
int                 libio_vfprintf( struct libio_file * stream, const char * format, va_list arg );
int                 libio_fgetc(    struct libio_file * stream );
int                 libio_fputc(    int c, struct libio_file * stream );
int                 libio_fputs(    const char * s, struct libio_file * stream );
size_t              libio_fread(    void * ptr, size_t size, size_t nobj, struct libio_file * stream );
size_t              libio_fwrite(   const void * ptr, size_t size, size_t nobj, struct libio_file * stream );
int                 libio_fseek(    struct libio_file * stream, long offset, int origin );
long                libio_ftell(    struct libio_file * stream );
void                libio_rewind(   struct libio_file * stream );
int                 libio_fgetpos(  struct libio_file * stream, fpos_t * ptr );
int                 libio_fsetpos(  struct libio_file * stream, const fpos_t * ptr );
void                libio_clearerr( struct libio_file * stream );
int                 libio_feof(     struct libio_file * stream );
int                 libio_ferror(   struct libio_file * stream );

/* Functions prototypes - Add-ons */
const char *        fname(          struct libio_file * stream );
const char *        fmode(          struct libio_file * stream );
bool                freadable(      struct libio_file * stream );
bool                fwriteable(     struct libio_file * stream );
bool                fcopy(          char * name, char * new_name );
int                 fgetbit(        struct libio_file * stream );
int                 fputbit(        struct libio_file * stream, uint8_t bit );
int                 fgetbits(       struct libio_file * stream, unsigned int count );
int                 fputbits(       struct libio_file * stream, uint64_t bits, unsigned int count );

/* Functions prototypes - Stat related functions */
dev_t               fdevid(         struct libio_file * stream );
ino_t               finoid(         struct libio_file * stream );
nlink_t             fnlink(         struct libio_file * stream );
uid_t               fuid(           struct libio_file * stream );
gid_t               fgid(           struct libio_file * stream );
size_t              fsize(          struct libio_file * stream );
double              fhsize(         struct libio_file * stream, char unit[] );
time_t              fatime(         struct libio_file * stream );
time_t              fmtime(         struct libio_file * stream );
time_t              fctime(         struct libio_file * stream );
bool                fblock(         struct libio_file * stream );
bool                fchar(          struct libio_file * stream );
bool                ffifo(          struct libio_file * stream );
bool                freg(           struct libio_file * stream );
bool                fdir(           struct libio_file * stream );
bool                flink(          struct libio_file * stream );
bool                fsock(          struct libio_file * stream );
bool                fuser_rwx(      struct libio_file * stream );
bool                fuser_r(        struct libio_file * stream );
bool                fuser_w(        struct libio_file * stream );
bool                fuser_x(        struct libio_file * stream );
bool                fgroup_rwx(     struct libio_file * stream );
bool                fgroup_r(       struct libio_file * stream );
bool                fgroup_w(       struct libio_file * stream );
bool                fgroup_x(       struct libio_file * stream );
bool                fother_rwx(      struct libio_file * stream );
bool                fother_r(       struct libio_file * stream );
bool                fother_w(       struct libio_file * stream );
bool                fother_x(       struct libio_file * stream );
bool                fsuid(          struct libio_file * stream );
bool                fsgid(          struct libio_file * stream );

/* Private functions */
static void __libio_write_align(  struct libio_file * stream );


static void __libio_write_align(  struct libio_file * stream )
{
    if( stream->bit_count > 0 )
    {
        fputc( stream->bit_buffer, stream->fp );
    }
    
    stream->bit_count  = 0;
    stream->bit_buffer = 0;
    stream->bit_offset = 0;
}

/*!
 * Opens file named filename and returns a stream, or NULL on failure.
 * Mode may be one of the following for text files:
 * 
 *      -   "r"     text reading
 *      -   "w"     text writing
 *      -   "a"     text append
 *      -   "r+"    text update (reading and writing)
 *      -   "w+"    text update discarding previous content (if any)
 *      -   "a+"    text append, reading, and writing at end
 * 
 * or one of those strings with b included (after the first character),
 * for binary files.
 */
struct libio_file * libio_fopen( const char * filename, const char * mode )
{
    FILE              * fp;
    struct libio_file * stream;
    
    if( NULL == ( fp = fopen( filename, mode ) ) )
    {
        return NULL;
    }
    
    if( NULL == ( stream = malloc( sizeof( struct libio_file ) ) ) )
    {
        return NULL;
    }
    
    stream->fp = fp;
    
    memset(  stream->filename,      0, FILENAME_MAX );
    memset(  stream->mode,          0, 4 );
    memset(  &( stream->stat_buf ), 0, sizeof( struct stat ) );
    strcpy(  stream->filename, filename );
    strncpy( stream->mode, mode, 3 );
    stat(    filename, &( stream->stat_buf ) );
    
    stream->need_init = false;
    stream->bit_buffer = 0;
    stream->bit_count  = 0;
    stream->bit_offset = 0;
    stream->readable   = false;
    stream->writeable  = false;
    
    if( strncmp( mode, "r", 1 ) == 0 )
    {
        stream->readable  = true;
        stream->writeable = ( strlen( mode ) > 1 && strncmp( mode + 1, "+", 1 ) ) ? true : false;
    }
    else if( strncmp( mode, "w", 1 ) == 0 )
    {
        stream->readable  = ( strlen( mode ) > 1 && strncmp( mode + 1, "+", 1 ) ) ? true : false;;
        stream->writeable = true;
    }
    else if( strncmp( mode, "a", 1 ) == 0 )
    {
        stream->readable  = ( strlen( mode ) > 1 && strncmp( mode + 1, "+", 1 ) ) ? true : false;;
        stream->writeable = true;
    }
    
    
    return stream;
}

/*!
 * Flushes stream stream and returns zero on success or EOF on error.
 * Effect undefined for input stream. fflush(NULL) flushes all output
 * streams.
 */
int libio_fflush( struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    
    return fflush( stream->fp );
}

/*!
 * Closes stream stream (after flushing, if output stream). Returns EOF on
 * error, zero otherwise.
 */
int libio_fclose( struct libio_file * stream )
{
    int res;
    
    __libio_write_align( stream );
    
    res = fclose( stream->fp );
    
    free( stream );
    
    return res;
}

/*!
 * Converts (according to format format) and writes output to stream stream.
 * Number of characters written, or negative value on error, is returned.
 * Conversion specifications consist of:
 *      
 *      -   %
 *      -   (optional) flag:
 *          
 *          -       left adjust
 *          +       always sign
 *          space   space if no sign
 *          0       zero pad
 *          #       Alternate form: for conversion character o, first digit will
 *                  be zero, for [xX], prefix 0x or 0X to non-zero value, for
 *                  [eEfgG], always decimal point, for [gG] trailing zeros not
 *                  removed.
 *          
 *      -   (optional) minimum width: if specified as *, value taken from next
 *          argument (which must be int).
 *      -   (optional) . (separating width from precision):
 *      -   (optional) precision: for conversion character s, maximum characters
 *          to be printed from the string, for [eEf], digits after decimal
 *          point, for [gG], significant digits, for an integer, minimum number
 *          of digits to be printed. If specified as *, value taken from next
 *          argument (which must be int).
 *      -   (optional) length modifier:
 *              
 *          h       short or unsigned short
 *          l       long or unsigned long
 *          L       long double
 *          
 *      conversion character:
 *          
 *          d,i     int argument, printed in signed decimal notation
 *          o       int argument, printed in unsigned octal notation
 *          x,X     int argument, printed in unsigned hexadecimal notation
 *          u       int argument, printed in unsigned decimal notation
 *          c       int argument, printed as single character
 *          s       char* argument
 *          f       double argument, printed with format [-]mmm.ddd
 *          e,E     double argument, printed with format [-]m.dddddd(e|E)(+|-)xx
 *          g,G     double argument
 *          p       void * argument, printed as pointer
 *          n       int * argument : the number of characters written to this
 *                  point is written into argument
 *          %       no argument; prints %  
 */
int libio_fprintf( struct libio_file * stream, const char * format, ... )
{
    int     res;
    va_list arg;
    
    __LIBIO_INIT( stream );
    __libio_write_align( stream );
    
    va_start( arg, format );
    
    res = vfprintf( stream->fp, format, arg );
    
    va_end( arg );
    
    return res;
}

/*!
 * Equivalent to fprintf with variable argument list replaced by arg, which must
 * have been initialised by the va_start macro (and may have been used in calls
 * to va_arg).
 */
int libio_vfprintf( struct libio_file * stream, const char * format, va_list arg )
{
    __LIBIO_INIT( stream );
    __libio_write_align( stream );
    
    return vfprintf( stream->fp, format, arg );
}

/*!
 * Returns next character from (input) stream stream, or EOF on end-of-file
 * or error.
 */
int libio_fgetc( struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    
    return fgetc( stream->fp );
}

/*!
 * Writes c, to stream stream. Returns c, or EOF on error.
 */
int libio_fputc( int c, struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    __libio_write_align( stream );
    
    return fputc( c, stream->fp );
}

/*!
 * Writes s, to (output) stream stream. Returns non-negative on success or
 * EOF on error.
 */
int libio_fputs( const char * s, struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    __libio_write_align( stream );
    
    return fputs( s, stream->fp );
}

/*!
 * Reads (at most) nobj objects of size size from stream stream into ptr and
 * returns number of objects read. (feof and ferror can be used to check
 * status.)
 */
size_t libio_fread( void * ptr, size_t size, size_t nobj, struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    
    return fread( ptr, size, nobj, stream->fp );
}

/*!
 * Writes to stream stream, nobj objects of size size from array ptr.
 * Returns number of objects written.
 */
size_t libio_fwrite( const void * ptr, size_t size, size_t nobj, struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    __libio_write_align( stream );
    
    return fwrite( ptr, size, nobj, stream->fp );
}

/*!
 * Sets file position for stream stream and clears end-of-file indicator.
 * For a binary stream, file position is set to offset bytes from the
 * position indicated by origin: beginning of file for SEEK_SET, current
 * position for SEEK_CUR, or end of file for SEEK_END. Behaviour is similar
 * for a text stream, but offset must be zero or, for SEEK_SET only, a value
 * returned by ftell.
 * Returns non-zero on error.
 */
int libio_fseek( struct libio_file * stream, long offset, int origin )
{
    __LIBIO_INIT( stream );
    
    return fseek( stream->fp, offset, origin );
}

/*!
 * Returns current file position for stream stream, or -1 on error.
 */
long libio_ftell( struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    
    return ftell( stream->fp );
}

/*!
 * Equivalent to fseek( stream, 0L, SEEK_SET ); clearerr( stream ).
 */
void libio_rewind( struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    
    rewind( stream->fp );
}

/*!
 * Stores current file position for stream stream in * ptr.
 * Returns non-zero on error.
 */
int libio_fgetpos( struct libio_file * stream, fpos_t * ptr )
{
    __LIBIO_INIT( stream );
    
    return fgetpos( stream->fp, ptr );
}

/*!
 * Sets current position of stream stream to * ptr.
 * Returns non-zero on error.
 */
int libio_fsetpos( struct libio_file * stream, const fpos_t * ptr )
{
    __LIBIO_INIT( stream );
    
    return fsetpos( stream->fp, ptr );
}

/*!
 * Clears end-of-file and error indicators for stream stream.
 */
void libio_clearerr( struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    
    clearerr( stream->fp );
}

/*!
 * Returns non-zero if end-of-file indicator is set for stream stream.
 */
int libio_feof( struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    
    return feof( stream->fp );
}

/*!
 * Returns non-zero if error indicator is set for stream stream.
 */
int libio_ferror( struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    
    return ferror( stream->fp );
}

/*!
 * Gets the filename associated to the stream-
 */
const char * fname( struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    
    if( stream->stdin == true )
    {
        return "stdin";
    }
    else if( stream->stdout == true )
    {
        return "stdout";
    }
    else if( stream->stderr == true )
    {
        return "stderr";
    }
    else
    {
        return stream->filename;
    }
}

/*!
 * Gets the stream's open mode
 */
const char * fmode( struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    
    if( stream->stdin == true )
    {
        return "r";
    }
    else if( stream->stdout == true )
    {
        return "a";
    }
    else if( stream->stderr == true )
    {
        return "a";
    }
    else
    {
        return stream->mode;
    }
}

/*!
 * Returns true if the file is readable
 */
bool freadable( struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    
    if( stream->stdin == true )
    {
        return true;
    }
    else if( stream->stdout == true )
    {
        return false;
    }
    else if( stream->stderr == true )
    {
        return false;
    }
    else
    {
        return stream->readable;
    }
}

/*!
 * Returns true if the file is writeable
 */
bool fwriteable( struct libio_file * stream )
{
    __LIBIO_INIT( stream );
    
    if( stream->stdin == true )
    {
        return false;
    }
    else if( stream->stdout == true )
    {
        return true;
    }
    else if( stream->stderr == true )
    {
        return true;
    }
    else
    {
        return stream->writeable;
    }
}

/*!
 * Copies a file to another destination.
 */
bool fcopy( char * name, char * new_name )
{
    FILE        * fp1;
    FILE        * fp2;
    size_t        length;
    unsigned char buffer[ 1024 ];
    
    if( NULL == ( fp1 = fopen( name, "rb" ) ) )
    {
        return false;
    }
    
    if( NULL == ( fp2 = fopen( name, "wb" ) ) )
    {
        fclose( fp1 );
        
        return false;
    }
    
    while( ( length = fread( buffer, sizeof( unsigned char ), 1024, fp1 ) ) )
    {
        if( fwrite( buffer, sizeof( unsigned char ), length, fp2 ) != length )
        {
            fclose( fp1 );
            fclose( fp2 );
            remove( new_name );
            
            return false;
        }
    }
    
    fclose( fp1 );
    fclose( fp2 );
    
    return true;
}

/*!
 * Gets the next bit in the stream
 */
int fgetbit( struct libio_file * stream )
{
    if( stream->readable == false )
    {
        return 0;
    }
    
    stream->bit_offset++;
    stream->bit_offset++;
    
    if( stream->bit_offset > 8 )
    {
        if( fread( &( stream->bit_buffer ), sizeof( char ), 1, stream->fp ) != 1 )
        {
            stream->bit_offset = 1;
            
            return -1;
        }
    }
    
    return ( stream->bit_buffer >> ( 8 - stream->bit_offset ) & 1 );
}

/*!
 * Writes a bit in the stream
 */
int fputbit( struct libio_file * stream, uint8_t bit )
{
    if( stream->writeable == false )
    {
        return -1;
    }
    
    stream->bit_buffer |= ( stream->bit_buffer << 1 | ( bit & 1 ) );
    stream->bit_count  += 1;
    
    if( stream->bit_count == 8 )
    {
        __libio_write_align( stream );
    }
    
    return 0;
}

/*!
 * Gets bits from the stream
 */
int fgetbits( struct libio_file * stream, unsigned int count )
{
    int          bits;
    unsigned int i;
    
    if( stream->readable == false )
    {
        return -1;
    }
    
    bits = 0;
    
    for( i = 0; i < count; i++ )
    {
        bits |= ( ( bits << 1 ) | fgetbit( stream ) );
    }
    
    return bits;
}

/*!
 * Writes bits to the stream
 */
int fputbits( struct libio_file * stream, uint64_t bits, unsigned int count )
{
    unsigned int i;
    uint8_t      bit;
    
    if( stream->writeable == false )
    {
        return -1;
    }
    
    for( i = 0; i < count; i++ )
    {
        bit = bits >> ( ( count - 1 ) - i );
        
        fputbit( stream, bit );
    }
    
    return 0;
}

/*!
 * 
 */
dev_t fdevid( struct libio_file * stream )
{
    return stream->stat_buf.st_dev;
}

/*!
 * 
 */
ino_t finoid( struct libio_file * stream )
{
    return stream->stat_buf.st_ino;
}

/*!
 * 
 */
nlink_t fnlink( struct libio_file * stream )
{
    return stream->stat_buf.st_nlink;
}

/*!
 * 
 */
uid_t fuid( struct libio_file * stream )
{
    return stream->stat_buf.st_uid;
}

/*!
 * 
 */
gid_t fgid( struct libio_file * stream )
{
    return stream->stat_buf.st_gid;
}

/*!
 * 
 */
size_t fsize( struct libio_file * stream )
{
    return stream->stat_buf.st_size;
}

/*!
 * 
 */
double fhsize( struct libio_file * stream, char unit[] )
{
    size_t        bytes;
    double        size;
    
    bytes = fsize( stream );
    
    memset( unit, 0, 3 );
    
    /* Checks the size range */
    if( bytes < 1000000 )
    {
        /* Kilo-Bytes */
        size = ( double )bytes / ( double )1000;
        
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

/*!
 * 
 */
time_t fatime( struct libio_file * stream )
{
    return stream->stat_buf.st_atime;
}

/*!
 * 
 */
time_t fmtime( struct libio_file * stream )
{
    return stream->stat_buf.st_mtime;
}

/*!
 * 
 */
time_t fctime( struct libio_file * stream )
{
    return stream->stat_buf.st_ctime;
}

/*!
 * 
 */
bool fblock( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IFMT ) == S_IFBLK ) ) ? true : false;
}

/*!
 * 
 */
bool fchar( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IFMT ) == S_IFCHR ) ) ? true : false;
}

/*!
 * 
 */
bool ffifo( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IFMT ) == S_IFIFO ) ) ? true : false;
}

/*!
 * 
 */
bool freg( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IFMT ) == S_IFREG ) ) ? true : false;
}

/*!
 * 
 */
bool fdir( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IFMT ) == S_IFDIR ) ) ? true : false;
}

/*!
 * 
 */
bool flink( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IFMT ) == S_IFLNK ) ) ? true : false;
}

/*!
 * 
 */
bool fsock( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IFMT ) == S_IFSOCK ) ) ? true : false;
}

/*!
 * 
 */
bool fuser_r( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IRWXU ) == S_IRUSR ) ) ? true : false;
}

/*!
 * 
 */
bool fuser_w( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IRWXU ) == S_IWUSR ) ) ? true : false;
}

/*!
 * 
 */
bool fuser_x( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IRWXU ) == S_IXUSR ) ) ? true : false;
}

/*!
 * 
 */
bool fgroup_r( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IRWXG ) == S_IRGRP ) ) ? true : false;
}

/*!
 * 
 */
bool fgroup_w( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IRWXG ) == S_IWGRP ) ) ? true : false;
}

/*!
 * 
 */
bool fgroup_x( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IRWXG ) == S_IXGRP ) ) ? true : false;
}

/*!
 * 
 */
bool fother_r( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IRWXO ) == S_IROTH ) ) ? true : false;
}

/*!
 * 
 */
bool fother_w( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IRWXO ) == S_IWOTH ) ) ? true : false;
}

/*!
 * 
 */
bool fother_x( struct libio_file * stream )
{
    return ( ( ( stream->stat_buf.st_mode & S_IRWXO ) == S_IXOTH ) ) ? true : false;
}

/*!
 * 
 */
bool fsuid( struct libio_file * stream )
{
    return ( stream->stat_buf.st_mode & S_ISUID ) ? true : false;
}

/*!
 * 
 */
bool fsgid( struct libio_file * stream )
{
    return ( stream->stat_buf.st_mode & S_ISGID ) ? true : false;
}
