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
 * @header      print.h
 * @copyright   eosgarden 2011 - Jean-David Gadina <macmade@eosgarden.com>
 * @abstract    Program's print functions
 */

#ifndef _EGZ_PRINT_H_
#define _EGZ_PRINT_H_
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

    /*!
     * 
     */
    void egz_print_table( egz_table * table );

    /*!
     * 
     */
    void egz_print_symbols( egz_symbol ** symbols, unsigned int count );

    /*!
     * 
     */
    void egz_print_node( egz_symbol * node );

    /*!
     * 
     */
    void egz_print_codes( egz_symbol ** symbols, unsigned int count );

    /*!
     * 
     */
    void egz_print_binary_code( egz_symbol * node );

    /*!
     * 
     */
    void egz_print_statistics( egz_symbol ** symbols, unsigned int count );

    /*!
     * 
     */
    void egz_print_file_ptr( FILE * fp );

#ifdef __cplusplus
}
#endif

#endif /* _EGZ_PRINT_H_ */