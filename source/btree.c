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
 * @file        btree.c
 * @copyright   (c) 2011 - Jean-David Gadina - www.xs-labs.com
 * @abstract    Binary tree functions
 */

/* Local includes */
#include "egz.h"


/*!
 * 
 */
egz_symbol * egz_create_btree( egz_symbol ** symbols, unsigned int count )
{
    unsigned int  i;
    unsigned int  j;
    unsigned int  k;
    bool          found;
    egz_symbol *  tree;
    egz_symbol *  s1;
    egz_symbol *  s2;
    egz_symbol *  s3;
    egz_symbol *  node;
    egz_symbol ** nodes;
    
    /* Allocates memory for the internal nodes */
    if( NULL == ( tree = ( egz_symbol * )malloc( sizeof( egz_symbol ) * ( count - 1 ) ) ) )
    {
        return NULL;
    }
    
    /* No need for the full process if there's only two symbols  */
    if( count == 2 )
    {
        s1               = *( symbols++ );
        s2               = *( symbols );
        tree->character  = 0;
        tree->occurences = s1->occurences + s2->occurences;
        tree->entropy    = 0;
        tree->id         = 1;
        tree->parent     = NULL;
        tree->left       = s2;
        tree->right      = s1;
        s1->parent       = tree;
        s2->parent       = tree;
        
        return tree;
    }
    
    /* Allocates memory to store pointers to the internal nodes */
    if( NULL == ( nodes = ( egz_symbol ** )malloc( sizeof( egz_symbol * ) * ( count - 1 ) ) ) )
    {
        free( tree );
        return NULL;
    }
    
    /* Moves the pointer to the last internal node */
    tree += count - 1;
    
    /* Process each internal node */
    for( i = 0; i < count - 1; i++ )
    {
        /* Data initialization */
        tree->character  = 0;
        tree->occurences = 0;
        tree->entropy    = 0;
        tree->id         = 0;
        tree->parent     = NULL;
        tree->left       = NULL;
        tree->right      = NULL;
        
        /* Stores the internal nodes ( in reverse order, so the tree will be the last used node) */
        nodes[ i ]       = --tree;
    }
    
    i = 0;
    j = 0;
    k = count - 1;
    
    /* Loops to create and connect the symbol nodes */
    while( i < count - 1 )
    {
        /* Gets the two next symbols */
        s1    = *( symbols++ );
        s2    = *( symbols++ );
        
        found = false;
        i    += 2;
        
        /* Process each internal nodes */
        for( j = 0; j < count - 1; j++ )
        {
            /* Current internal node */
            node = nodes[ j ];
            
            /* Node is not available (already used) */
            if( node == NULL || node->parent != NULL )
            {
                continue;
            }
            
            /* If the node has no occurence number, it's available to use */
            if( node->occurences == 0 )
            {
                /* Stores the node ID */
                node->id = k--;
                
                /* Stores the node left and right children */
                node->left  = s2;
                node->right = s1;
                
                /* Computes the internal node occurences */
                node->occurences = s1->occurences + s2->occurences;
                
                /* Stores the parent node for the two children */
                s1->parent = node;
                s2->parent = node;
                
                /* Prints the internal node representation */
                if( libdebug_is_enabled() == true )
                {
                    DEBUG( "Creating new internal node - [%03u]:", node->id );
                    egz_print_node( node );
                }
                
                /* Nodes have been connected - Next symbols can be processed */
                break;
            }
            
            /* We have found a connection - Continues until we find an available internal node */
            else if( found == true && node->occurences != 0 )
            {
                continue;
            }
            
            /* Checks if we can connect the internal node with another internal node */
            /* This is possible if its occurence number us lower than the second's symbol occurence number  */
            if( node->occurences < s2->occurences )
            {
                /* Sets the pointer to NULL to mark it unavailable */
                nodes[ j ] = NULL;
                
                /* The connection will be made with the internal node instead of the second symbol */
                s2    = node;
                found = true;
                
                /* Rewinds, as we are not using the second symbol */
                i--;
                symbols--;
            }
        }
        
        found = false;
    }
    
    /* (Re)Initialization */
    node = NULL;
    s1   = NULL;
    s2   = NULL;
    s3   = NULL;
    
    /* Checks if we still have a symbol not connected to a node */
    if( i < count )
    {
        /* Last (not connected) symbol */
        s3 = *( symbols );
        DEBUG( "Last symbol: 0x%0X / %u", s3->character, s3->occurences );
    }
    
    DEBUG( "Connecting final nodes" );
    
    /* No need for the full process if there's only one node left  */
    if( k == 1 )
    {
        /* Checks if we have a symbol left */
        if( s3 != NULL )
        {
            s1 = s3;
            s2 = nodes[ j++ ];
        }
        else
        {
            s1 = nodes[ j - 1 ];
            s2 = nodes[ j++ ];
        }
        
        node             = nodes[ j ];
        node->id         = k--;
        node->left       = s2;
        node->right      = s1;
        s1->parent       = node;
        s2->parent       = node;
        node->occurences = s1->occurences + s2->occurences;
    }
    else
    {
        i = 0;
        j = 0;
        
        while( i < count - 4 )
        {
            do
            {
                s1         = nodes[ i ];
                nodes[ i++ ] = NULL;
            }
            while( s1 == NULL );
            
            do
            {
                s2         = nodes[ i ];
                nodes[ i++ ] = NULL;
            }
            while( s2 == NULL );
            
            if( s3 != NULL && s2->occurences >= s3->occurences )
            {
                nodes[ --i ] = s2;
                s2           = s1;
                s1           = s3;
                s3           = NULL;
            }
            
            if( j == 0 )
            {
                for( j = i; j < count - 1; j++ )
                {
                    if( nodes[ j ]->occurences == 0 )
                    {
                        node = nodes[ j ];
                        break;
                    }
                }
            }
            else
            {
                node = nodes[ ++j ];
            }
            
            node->id         = k--;
            node->left       = s1;
            node->right      = s2;
            s1->parent       = node;
            s2->parent       = node;
            node->occurences = s1->occurences + s2->occurences;
            
            if( libdebug_is_enabled() == true )
            {
                DEBUG( "Creating new internal node - [%03u]:", node->id );
                egz_print_node( node );
            }
        }
        
        if( s3 == NULL )
        {
            s1 = nodes[ j - 1 ];
            s2 = nodes[ j++ ];
        }
        else
        {
            s1 = s3;
            s2 = nodes[ j++ ];
        }
        
        node             = nodes[ j ];
        node->id         = k--;
        node->left       = s2;
        node->right      = s1;
        s1->parent       = node;
        s2->parent       = node;
        node->occurences = s1->occurences + s2->occurences;
    }
    
    if( libdebug_is_enabled() == true )
    {
        DEBUG( "Creating root node - [%03u]:", node->id );
        egz_print_node( node );
    }
    
    DEBUG( "Freeing memory" );
    free( nodes );
    
    return tree;
}

/*!
 * 
 */
void egz_create_codes( egz_symbol * node, unsigned int depth, uint64_t code )
{
    /* Checks if we have a leaf node */
    if( node->left == NULL && node->right == NULL )
    {
        /* Leaf - Stores the depth and the binary code */
        node->bits = depth;
        node->code = code;
    }
    else
    {
        /* Node - Creates codes for the left (0) and right (1) children */
        egz_create_codes( node->left,  depth + 1, ( code << 1 ) );
        egz_create_codes( node->right, depth + 1, ( code << 1 ) + 1 );
    }
}
