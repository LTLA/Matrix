#ifndef CHM_COMMON_H
#define CHM_COMMON_H

#include "UFconfig/UFconfig.h"
#include "CHOLMOD/Include/cholmod.h"
#include "Mutils.h"
#ifdef Matrix_with_SPQR
#  include "SPQR/Include/SuiteSparseQR_C.h"
#endif

typedef struct cholmod_common_struct  *CHM_CM ;
typedef struct cholmod_dense_struct   *CHM_DN ;
typedef struct cholmod_factor_struct  *CHM_FR ;
typedef struct cholmod_sparse_struct  *CHM_SP ;
typedef struct cholmod_triplet_struct *CHM_TR ;

extern cholmod_common c;	/* structure for int CHM routines */
extern cholmod_common cl;	/* structure for UF_long routines */

/* NOTE: Versions of these are *EXPORTED* via ../inst/include/Matrix.h
 * ----  and used e.g., in the lme4 package
 */
CHM_SP as_cholmod_sparse (CHM_SP ans, SEXP x, Rboolean check_Udiag, Rboolean sort_in_place);
CHM_TR as_cholmod_triplet(CHM_TR ans, SEXP x, Rboolean check_Udiag);
CHM_DN as_cholmod_dense  (CHM_DN ans, SEXP x);
CHM_DN as_cholmod_x_dense(CHM_DN ans, SEXP x);
CHM_DN numeric_as_chm_dense(CHM_DN ans, double *v, int nr, int nc);
CHM_FR as_cholmod_factor (CHM_FR ans, SEXP x);

/* Deep copy to a cholmod_l struct, not just copying pointers.
 * Must be freed with cholmod_l_free_sparse.
 * Maybe not.  Need more discussion.
CHM_SP SEXP_to_cholmod_l_sparse(SEXP x);
 */

#define AS_CHM_DN(x) as_cholmod_dense  ((CHM_DN)alloca(sizeof(cholmod_dense)), x )
#define AS_CHM_FR(x) as_cholmod_factor ((CHM_FR)alloca(sizeof(cholmod_factor)), x )
#define AS_CHM_SP(x) as_cholmod_sparse ((CHM_SP)alloca(sizeof(cholmod_sparse)), x, TRUE, FALSE)
#define AS_CHM_TR(x) as_cholmod_triplet((CHM_TR)alloca(sizeof(cholmod_triplet)),x, TRUE)
/* the non-diagU2N-checking versions : */
#define AS_CHM_SP__(x) as_cholmod_sparse ((CHM_SP)alloca(sizeof(cholmod_sparse)), x, FALSE, FALSE)
#define AS_CHM_TR__(x) as_cholmod_triplet((CHM_TR)alloca(sizeof(cholmod_triplet)), x, FALSE)


#define N_AS_CHM_DN(x,nr,nc) M_numeric_as_chm_dense((CHM_DN)alloca(sizeof(cholmod_dense)), x , nr, nc )

Rboolean check_sorted_chm(CHM_SP A);

int R_cholmod_start(CHM_CM Common);
int R_cholmod_l_start(CHM_CM Common);
void R_cholmod_error(int status, const char *file, int line, const char *message);

SEXP chm_factor_to_SEXP(CHM_FR f, int dofree);
SEXP chm_sparse_to_SEXP(CHM_SP a, int dofree, int uploT, int Rkind,
			const char *diag, SEXP dn);
SEXP chm_triplet_to_SEXP(CHM_TR a, int dofree, int uploT, int Rkind,
			 const char* diag, SEXP dn);
SEXP chm_dense_to_SEXP(CHM_DN a, int dofree, int Rkind, SEXP dn);
/* 		       int uploST, char *diag, SEXP dn); */
SEXP chm_dense_to_matrix(CHM_DN a, int dofree, SEXP dn);

void chm_diagN2U(CHM_SP chx, int uploT, Rboolean do_realloc);

SEXP CHMfactor_validate(SEXP obj);
SEXP CHMsimpl_validate(SEXP obj);
SEXP CHMsuper_validate(SEXP obj);

SEXP CHM_store_common(SEXP rho);
SEXP CHM_restore_common(SEXP rho);
#endif
