/* double (precision) TRiangular Matrices */

#include "dtrMatrix.h"

/* FIXME: dtrMatrix_as_dgeMatrix()  {below}
 * -----  is called *before* the following - presumably in order to
 *        apply the higher level validation first
*/
SEXP dtrMatrix_validate(SEXP obj)
{
    SEXP val;

    if (isString(val = check_scalar_string(GET_SLOT(obj, Matrix_uploSym),
					   "LU", "uplo"))) return val;
    if (isString(val = check_scalar_string(GET_SLOT(obj, Matrix_diagSym),
					   "NU", "diag"))) return val;
    return ScalarLogical(1);
}

static
double get_norm(SEXP obj, char *typstr)
{
    char typnm[] = {'\0', '\0'};
    int *dims = INTEGER(GET_SLOT(obj, Matrix_DimSym));
    double *work = (double *) NULL;

    typnm[0] = norm_type(typstr);
    if (*typnm == 'I') {
	work = (double *) R_alloc(dims[0], sizeof(double));
    }
    return F77_CALL(dlantr)(typnm,
			    CHAR(asChar(GET_SLOT(obj, Matrix_uploSym))),
			    CHAR(asChar(GET_SLOT(obj, Matrix_diagSym))),
			    dims, dims+1,
			    REAL(GET_SLOT(obj, Matrix_xSym)),
			    dims, work);
}


SEXP dtrMatrix_norm(SEXP obj, SEXP type)
{
    return ScalarReal(get_norm(obj, CHAR(asChar(type))));
}

static
double set_rcond(SEXP obj, char *typstr)
{
    char typnm[] = {'\0', '\0'};
    SEXP rcv = GET_SLOT(obj, Matrix_rcondSym);
    double rcond = get_double_by_name(rcv, typnm);

    typnm[0] = rcond_type(typstr);
    if (R_IsNA(rcond)) {
	int *dims = INTEGER(GET_SLOT(obj, Matrix_DimSym)), info;
	F77_CALL(dtrcon)(typnm,
			 CHAR(asChar(GET_SLOT(obj, Matrix_uploSym))),
			 CHAR(asChar(GET_SLOT(obj, Matrix_diagSym))),
			 dims, REAL(GET_SLOT(obj, Matrix_xSym)),
			 dims, &rcond,
			 (double *) R_alloc(3*dims[0], sizeof(double)),
			 (int *) R_alloc(dims[0], sizeof(int)), &info);
	SET_SLOT(obj, Matrix_rcondSym,
		 set_double_by_name(rcv, rcond, typnm));
    }
    return rcond;
}

SEXP dtrMatrix_rcond(SEXP obj, SEXP type)
{
    return ScalarReal(set_rcond(obj, CHAR(asChar(type))));
}

SEXP dtrMatrix_solve(SEXP a)
{
    SEXP val = PROTECT(duplicate(a));
    int info, *Dim = INTEGER(GET_SLOT(val, Matrix_DimSym));
    F77_CALL(dtrtri)(CHAR(asChar(GET_SLOT(val, Matrix_uploSym))),
		     CHAR(asChar(GET_SLOT(val, Matrix_diagSym))),
		     Dim, REAL(GET_SLOT(val, Matrix_xSym)), Dim, &info);
    UNPROTECT(1);
    return val;
}

SEXP dtrMatrix_matrix_solve(SEXP a, SEXP b)
{
    SEXP val = PROTECT(duplicate(b));
    int *Dim = INTEGER(GET_SLOT(a, Matrix_DimSym)),
	*bDim = INTEGER(getAttrib(val, R_DimSymbol));
    double one = 1.0;

    if (bDim[0] != Dim[1])
	error(_("Dimensions of a (%d,%d) and b (%d,%d) do not conform"),
	      Dim[0], Dim[1], bDim[0], bDim[1]);
    F77_CALL(dtrsm)("L", CHAR(asChar(GET_SLOT(val, Matrix_uploSym))),
		    "N", CHAR(asChar(GET_SLOT(val, Matrix_diagSym))),
		    bDim, bDim+1, &one,
		    REAL(GET_SLOT(a, Matrix_xSym)), Dim,
		    REAL(val), bDim);
    UNPROTECT(1);
    return val;
}

SEXP dtrMatrix_as_dgeMatrix(SEXP from)
{
    SEXP val = PROTECT(NEW_OBJECT(MAKE_CLASS("dgeMatrix")));

    SET_SLOT(val, Matrix_rcondSym,
	     duplicate(GET_SLOT(from, Matrix_rcondSym)));
    SET_SLOT(val, Matrix_xSym, duplicate(GET_SLOT(from, Matrix_xSym)));
    /* Dim < 2 can give a seg.fault problem in make_array_triangular(): */
    if (LENGTH(GET_SLOT(from, Matrix_DimSym)) < 2)
	error(_(_("'Dim' slot has length less than two")));
    SET_SLOT(val, Matrix_DimSym,
	     duplicate(GET_SLOT(from, Matrix_DimSym)));
    SET_SLOT(val, Matrix_factorSym, allocVector(VECSXP, 0));
    make_array_triangular(REAL(GET_SLOT(val, Matrix_xSym)), from);
    UNPROTECT(1);
    return val;
}

SEXP dtrMatrix_as_matrix(SEXP from)
{
    int *Dim = INTEGER(GET_SLOT(from, Matrix_DimSym));
    int m = Dim[0], n = Dim[1];
    SEXP val = PROTECT(allocMatrix(REALSXP, m, n));

    make_array_triangular(Memcpy(REAL(val),
				 REAL(GET_SLOT(from, Matrix_xSym)), m * n),
			  from);
    UNPROTECT(1);
    return val;
}

SEXP dtrMatrix_getDiag(SEXP x)
{
    int i, n = INTEGER(GET_SLOT(x, Matrix_DimSym))[0];
    SEXP ret = PROTECT(allocVector(REALSXP, n)),
	xv = GET_SLOT(x, Matrix_xSym);

    if ('U' == CHAR(STRING_ELT(GET_SLOT(x, Matrix_diagSym), 0))[0]) {
	for (i = 0; i < n; i++) REAL(ret)[i] = 1.;
    } else {
	for (i = 0; i < n; i++) {
	    REAL(ret)[i] = REAL(xv)[i * (n + 1)];
	}
    }
    UNPROTECT(1);
    return ret;
}

SEXP dtrMatrix_dgeMatrix_mm(SEXP a, SEXP b)
{
    int *adims = INTEGER(GET_SLOT(a, Matrix_DimSym)),
	*bdims = INTEGER(GET_SLOT(b, Matrix_DimSym)),
	m = adims[0], n = bdims[1], k = adims[1];
    SEXP val = PROTECT(duplicate(b));
    double one = 1.;

    if (bdims[0] != k)
	error(_("Matrices are not conformable for multiplication"));
    if (m < 1 || n < 1 || k < 1)
	error(_("Matrices with zero extents cannot be multiplied"));
    F77_CALL(dtrmm)("L", CHAR(asChar(GET_SLOT(a, Matrix_uploSym))), "N",
		    CHAR(asChar(GET_SLOT(a, Matrix_diagSym))),
		    adims, bdims+1, &one,
		    REAL(GET_SLOT(a, Matrix_xSym)), adims,
		    REAL(GET_SLOT(val, Matrix_xSym)), bdims);
    UNPROTECT(1);
    return val;
}

SEXP dtrMatrix_dgeMatrix_mm_R(SEXP a, SEXP b)
{
    int *adims = INTEGER(GET_SLOT(a, Matrix_DimSym)),
	*bdims = INTEGER(GET_SLOT(b, Matrix_DimSym)),
	m = adims[0], n = bdims[1], k = adims[1];
    SEXP val = PROTECT(duplicate(b));
    double one = 1.;

    if (bdims[0] != k)
	error(_("Matrices are not conformable for multiplication"));
    if (m < 1 || n < 1 || k < 1)
	error(_("Matrices with zero extents cannot be multiplied"));
    F77_CALL(dtrmm)("R", CHAR(asChar(GET_SLOT(a, Matrix_uploSym))), "N",
		    CHAR(asChar(GET_SLOT(a, Matrix_diagSym))),
		    adims, bdims+1, &one,
		    REAL(GET_SLOT(a, Matrix_xSym)), adims,
		    REAL(GET_SLOT(val, Matrix_xSym)), bdims);
    UNPROTECT(1);
    return val;
}

SEXP dtrMatrix_as_dtpMatrix(SEXP from)
{
    SEXP val = PROTECT(NEW_OBJECT(MAKE_CLASS("dtpMatrix"))),
	uplo = GET_SLOT(from, Matrix_uploSym),
	diag = GET_SLOT(from, Matrix_diagSym),
	dimP = GET_SLOT(from, Matrix_DimSym);
    int n = *INTEGER(dimP);

    SET_SLOT(val, Matrix_rcondSym,
	     duplicate(GET_SLOT(from, Matrix_rcondSym)));
    SET_SLOT(val, Matrix_DimSym, duplicate(dimP));
    SET_SLOT(val, Matrix_diagSym, duplicate(diag));
    SET_SLOT(val, Matrix_uploSym, duplicate(uplo));
    full_to_packed(REAL(ALLOC_SLOT(val, Matrix_xSym, REALSXP, (n*(n+1))/2)),
		   REAL(GET_SLOT(from, Matrix_xSym)), n,
		   *CHAR(STRING_ELT(uplo, 0)) == 'U' ? UPP : LOW,
		   *CHAR(STRING_ELT(diag, 0)) == 'U' ? UNT : NUN);
    UNPROTECT(1);
    return val;
}
