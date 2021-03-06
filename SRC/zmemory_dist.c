/*! \file
Copyright (c) 2003, The Regents of the University of California, through
Lawrence Berkeley National Laboratory (subject to receipt of any required 
approvals from U.S. Dept. of Energy) 

All rights reserved. 

The source code is distributed under BSD license, see the file License.txt
at the top-level directory.
*/

/*! @file
 * \brief Memory utilities
 *
 * <pre>
 * -- Distributed SuperLU routine (version 4.0) --
 * Lawrence Berkeley National Lab, Univ. of California Berkeley.
 * October 1, 2014
 * </pre>
 */

#include "superlu_zdefs.h"


/* Variables external to this file */
extern LU_stack_t stack;


void *zuser_malloc_dist(int_t bytes, int_t which_end)
{
    void *buf;
    
    if ( StackFull(bytes) ) return (NULL);

    if ( which_end == HEAD ) {
	buf = (char*) stack.array + stack.top1;
	stack.top1 += bytes;
    } else {
	stack.top2 -= bytes;
	buf = (char*) stack.array + stack.top2;
    }
    
    stack.used += bytes;
    return buf;
}


void zuser_free_dist(int_t bytes, int_t which_end)
{
    if ( which_end == HEAD ) {
	stack.top1 -= bytes;
    } else {
	stack.top2 += bytes;
    }
    stack.used -= bytes;
}



/*! \brief
 *
 * <pre>
 * mem_usage consists of the following fields:
 *    - for_lu (float)
 *      The amount of space used in bytes for the L\U data structures.
 *    - total (float)
 *      The amount of space needed in bytes to perform factorization.
 *    - expansions (int)
 *      Number of memory expansions during the LU factorization.
 * </pre>
 */
int_t zQuerySpace_dist(int_t n, LUstruct_t *LUstruct, gridinfo_t *grid,
		       SuperLUStat_t *stat, superlu_dist_mem_usage_t *mem_usage)
{
    register int_t dword, gb, iword, k, nb, nsupers;
    int_t *index, *xsup;
    int iam, mycol, myrow;
    Glu_persist_t *Glu_persist = LUstruct->Glu_persist;
    LocalLU_t *Llu = LUstruct->Llu;

    iam = grid->iam;
    myrow = MYROW( iam, grid );
    mycol = MYCOL( iam, grid );
    iword = sizeof(int_t);
    dword = sizeof(doublecomplex);
    nsupers = Glu_persist->supno[n-1] + 1;
    xsup = Glu_persist->xsup;
    mem_usage->for_lu = 0.;

    /* For L factor */
    nb = CEILING( nsupers, grid->npcol ); /* Number of local column blocks */
    for (k = 0; k < nb; ++k) {
	gb = k * grid->npcol + mycol; /* Global block number. */
	if ( gb < nsupers ) {
	    index = Llu->Lrowind_bc_ptr[k];
	    if ( index ) {
		mem_usage->for_lu += (float)
		    ((BC_HEADER + index[0]*LB_DESCRIPTOR + index[1]) * iword);
		mem_usage->for_lu += (float)(index[1]*SuperSize( gb )*dword);
	    }
	}
    }

    /* For U factor */
    nb = CEILING( nsupers, grid->nprow ); /* Number of local row blocks */
    for (k = 0; k < nb; ++k) {
	gb = k * grid->nprow + myrow; /* Global block number. */
	if ( gb < nsupers ) {
	    index = Llu->Ufstnz_br_ptr[k];
	    if ( index ) {
		mem_usage->for_lu += (float)(index[2] * iword);
		mem_usage->for_lu += (float)(index[1] * dword);
	    }
	}
    }

    /* Working storage to support factorization */
    mem_usage->total = mem_usage->for_lu;
#if 0
    mem_usage->total +=
	(float)(( Llu->bufmax[0] + Llu->bufmax[2] ) * iword +
		( Llu->bufmax[1] + Llu->bufmax[3] + maxsup ) * dword );
    /**** another buffer to use mpi_irecv in pdgstrf_irecv.c ****/
    mem_usage->total +=
	(float)( Llu->bufmax[0] * iword +  Llu->bufmax[1] * dword );
    mem_usage->total += (float)( maxsup * maxsup + maxsup) * iword;
    k = CEILING( nsupers, grid->nprow );
    mem_usage->total += (float)(2 * k * iword);
#else
    /*mem_usage->total += stat->current_buffer;*/
    printf(".. zQuery_Space: peak_buffer %.2f (MB)\n", stat->peak_buffer * 1.0e-6);
    mem_usage->total += stat->peak_buffer;
#endif

    return 0;
} /* zQuerySpace_dist */


/*
 * Allocate storage for original matrix A
 */
void
zallocateA_dist(int_t n, int_t nnz, doublecomplex **a, int_t **asub, int_t **xa)
{
    *a    = (doublecomplex *) doublecomplexMalloc_dist(nnz);
    *asub = (int_t *) intMalloc_dist(nnz);
    *xa   = (int_t *) intMalloc_dist(n+1);
}


doublecomplex *doublecomplexMalloc_dist(int_t n)
{
    doublecomplex *buf;
    buf = (doublecomplex *) SUPERLU_MALLOC( SUPERLU_MAX(1, n) * sizeof(doublecomplex) ); 
    return (buf);
}

doublecomplex *doublecomplexCalloc_dist(int_t n)
{
    doublecomplex *buf;
    register int_t i;
    doublecomplex zero = {0.0, 0.0};
    buf = (doublecomplex *) SUPERLU_MALLOC( SUPERLU_MAX(1, n) * sizeof(doublecomplex));
    if ( !buf ) return (buf);
    for (i = 0; i < n; ++i) buf[i] = zero;
    return (buf);
}

