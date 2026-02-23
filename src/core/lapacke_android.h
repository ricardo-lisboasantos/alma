#ifndef LAPACKE_ANDROID_H
#define LAPACKE_ANDROID_H

#include <stdint.h>

typedef int lapack_int;
typedef int lapack_logical;

#define LAPACK_ROW_MAJOR               'R'
#define LAPACK_COL_MAJOR               'C'

#ifdef __cplusplus
extern "C" {
#endif

lapack_int LAPACKE_dgeev(int matrix_layout, char jobvl, char jobvr, lapack_int n,
                          double* a, lapack_int lda, double* wr, double* wi,
                          double* vl, lapack_int ldvl, double* vr, lapack_int ldvr);

lapack_int LAPACKE_sgeev(int matrix_layout, char jobvl, char jobvr, lapack_int n,
                          float* a, lapack_int lda, float* wr, float* wi,
                          float* vl, lapack_int ldvl, float* vr, lapack_int ldvr);

lapack_int LAPACKE_dgetrf(int matrix_layout, lapack_int m, lapack_int n,
                            double* a, lapack_int lda, lapack_int* ipiv);

lapack_int LAPACKE_sgetrf(int matrix_layout, lapack_int m, lapack_int n,
                            float* a, lapack_int lda, lapack_int* ipiv);

lapack_int LAPACKE_dgetri(int matrix_layout, lapack_int n, double* a,
                           lapack_int lda, const lapack_int* ipiv);

lapack_int LAPACKE_sgetri(int matrix_layout, lapack_int n, float* a,
                           lapack_int lda, const lapack_int* ipiv);

lapack_int LAPACKE_dgetri_work(int matrix_layout, lapack_int n, double* a,
                                lapack_int lda, const lapack_int* ipiv,
                                double* work, lapack_int lwork);

lapack_int LAPACKE_sgetri_work(int matrix_layout, lapack_int n, float* a,
                                lapack_int lda, const lapack_int* ipiv,
                                float* work, lapack_int lwork);

lapack_int LAPACKE_dgetrs(int matrix_layout, char trans, lapack_int n,
                           lapack_int nrhs, const double* a, lapack_int lda,
                           const lapack_int* ipiv, double* b, lapack_int ldb);

lapack_int LAPACKE_sgetrs(int matrix_layout, char trans, lapack_int n,
                           lapack_int nrhs, const float* a, lapack_int lda,
                           const lapack_int* ipiv, float* b, lapack_int ldb);

lapack_int LAPACKE_dgels(int matrix_layout, char trans, lapack_int m,
                           lapack_int n, lapack_int nrhs, double* a,
                           lapack_int lda, double* b, lapack_int ldb);

lapack_int LAPACKE_sgels(int matrix_layout, char trans, lapack_int m,
                           lapack_int n, lapack_int nrhs, float* a,
                           lapack_int lda, float* b, lapack_int ldb);

lapack_int LAPACKE_dorgqr(int matrix_layout, lapack_int m, lapack_int n,
                           lapack_int k, double* a, lapack_int lda,
                           const lapack_int* tau);

lapack_int LAPACKE_sorgqr(int matrix_layout, lapack_int m, lapack_int n,
                           lapack_int k, float* a, lapack_int lda,
                           const lapack_int* tau);

lapack_int LAPACKE_dorgqr_work(int matrix_layout, lapack_int m, lapack_int n,
                                lapack_int k, double* a, lapack_int lda,
                                const double* tau, double* work, lapack_int lwork);

lapack_int LAPACKE_sorgqr_work(int matrix_layout, lapack_int m, lapack_int n,
                                lapack_int k, float* a, lapack_int lda,
                                const float* tau, float* work, lapack_int lwork);

lapack_int LAPACKE_dgeqrf_work(int matrix_layout, lapack_int m, lapack_int n,
                                double* a, lapack_int lda, double* tau,
                                double* work, lapack_int lwork);

lapack_int LAPACKE_sgeqrf_work(int matrix_layout, lapack_int m, lapack_int n,
                                float* a, lapack_int lda, float* tau,
                                float* work, lapack_int lwork);

lapack_int LAPACKE_dgesvd(int matrix_layout, char jobu, char jobvt, lapack_int m,
                           lapack_int n, double* a, lapack_int lda, double* s,
                           double* u, lapack_int ldu, double* vt, lapack_int ldvt,
                           double* superb);

lapack_int LAPACKE_sgesvd(int matrix_layout, char jobu, char jobvt, lapack_int m,
                           lapack_int n, float* a, lapack_int lda, float* s,
                           float* u, lapack_int ldu, float* vt, lapack_int ldvt,
                           float* superb);

lapack_int LAPACKE_dgesdd(int matrix_layout, char jobz, lapack_int m,
                           lapack_int n, double* a, lapack_int lda,
                           double* s, double* u, lapack_int ldu,
                           double* vt, lapack_int ldvt);

lapack_int LAPACKE_sgesdd(int matrix_layout, char jobz, lapack_int m,
                           lapack_int n, float* a, lapack_int lda,
                           float* s, float* u, lapack_int ldu,
                           float* vt, lapack_int ldvt);

lapack_int LAPACKE_dgebrd(int matrix_layout, lapack_int m, lapack_int n,
                           double* a, lapack_int lda, double* d,
                           double* e, double* tauq, double* taup);

lapack_int LAPACKE_sgebrd(int matrix_layout, lapack_int m, lapack_int n,
                           float* a, lapack_int lda, float* d,
                           float* e, float* tauq, float* taup);

lapack_int LAPACKE_dgebak(int matrix_layout, char job, char side,
                           lapack_int n, lapack_int ilo, lapack_int ihi,
                           const double* scale, lapack_int m, double* v,
                           lapack_int ldv);

lapack_int LAPACKE_sgebak(int matrix_layout, char job, char side,
                           lapack_int n, lapack_int ilo, lapack_int ihi,
                           const float* scale, lapack_int m, float* v,
                           lapack_int ldv);

lapack_int LAPACKE_dgebal(int matrix_layout, char job, lapack_int n,
                           double* a, lapack_int lda, lapack_int* ilo,
                           lapack_int* ihi, double* scale);

lapack_int LAPACKE_sgebal(int matrix_layout, char job, lapack_int n,
                           float* a, lapack_int lda, lapack_int* ilo,
                           lapack_int* ihi, float* scale);

lapack_int LAPACKE_dgehrd(int matrix_layout, lapack_int n, lapack_int ilo,
                           lapack_int ihi, double* a, lapack_int lda,
                           const double* tau);

lapack_int LAPACKE_sgehrd(int matrix_layout, lapack_int n, lapack_int ilo,
                           lapack_int ihi, float* a, lapack_int lda,
                           const float* tau);

lapack_int LAPACKE_dorghr(int matrix_layout, lapack_int n, lapack_int ilo,
                           lapack_int ihi, double* a, lapack_int lda,
                           const double* tau);

lapack_int LAPACKE_sorghr(int matrix_layout, lapack_int n, lapack_int ilo,
                           lapack_int ihi, float* a, lapack_int lda,
                           const float* tau);

lapack_int LAPACKE_dhseqr(int matrix_layout, char job, char compz, lapack_int n,
                           lapack_int ilo, lapack_int ihi, double* h,
                           lapack_int ldh, double* wr, double* wi, double* z,
                           lapack_int ldz);

lapack_int LAPACKE_shseqr(int matrix_layout, char job, char compz, lapack_int n,
                           lapack_int ilo, lapack_int ihi, float* h,
                           lapack_int ldh, float* wr, float* wi, float* z,
                           lapack_int ldz);

lapack_int LAPACKE_dlange(int matrix_layout, char norm, lapack_int m,
                           lapack_int n, const double* a, lapack_int lda,
                           double* work);

lapack_int LAPACKE_slange(int matrix_layout, char norm, lapack_int m,
                           lapack_int n, const float* a, lapack_int lda,
                           float* work);

lapack_int LAPACKE_dlanhe(int matrix_layout, char norm, char uplo,
                           lapack_int n, const double* a, lapack_int lda,
                           double* work);

lapack_int LAPACKE_slansy(int matrix_layout, char norm, char uplo,
                           lapack_int n, const float* a, lapack_int lda,
                           float* work);

lapack_int LAPACKE_dlansy(int matrix_layout, char norm, char uplo,
                           lapack_int n, const double* a, lapack_int lda,
                           double* work);

lapack_int LAPACKE_dpotrf(int matrix_layout, char uplo, lapack_int n,
                           double* a, lapack_int lda);

lapack_int LAPACKE_spotrf(int matrix_layout, char uplo, lapack_int n,
                           float* a, lapack_int lda);

lapack_int LAPACKE_dpotri(int matrix_layout, char uplo, lapack_int n,
                           double* a, lapack_int lda);

lapack_int LAPACKE_spotri(int matrix_layout, char uplo, lapack_int n,
                           float* a, lapack_int lda);

lapack_int LAPACKE_dpotrs(int matrix_layout, char uplo, lapack_int n,
                           lapack_int nrhs, const double* a, lapack_int lda,
                           double* b, lapack_int ldb);

lapack_int LAPACKE_spotrs(int matrix_layout, char uplo, lapack_int n,
                           lapack_int nrhs, const float* a, lapack_int lda,
                           float* b, lapack_int ldb);

lapack_int LAPACKE_dsyev(int matrix_layout, char jobz, char uplo, lapack_int n,
                          double* a, lapack_int lda, double* w, double* z,
                          lapack_int ldz);

lapack_int LAPACKE_ssyev(int matrix_layout, char jobz, char uplo, lapack_int n,
                          float* a, lapack_int lda, float* w, float* z,
                          lapack_int ldz);

lapack_int LAPACKE_dgeevx(int matrix_layout, char balanc, char jobvl, char jobvr,
                           char sense, lapack_int n, double* a, lapack_int lda,
                           double* wr, double* wi, double* vl, lapack_int ldvl,
                           double* vr, lapack_int ldvr, lapack_int* ilo,
                           lapack_int* ihi, double* scale, double* abnrm,
                           double* rconde, double* rcondv);

lapack_int LAPACKE_sgeevx(int matrix_layout, char balanc, char jobvl, char jobvr,
                           char sense, lapack_int n, float* a, lapack_int lda,
                           float* wr, float* wi, float* vl, lapack_int ldvl,
                           float* vr, lapack_int ldvr, lapack_int* ilo,
                           lapack_int* ihi, float* scale, float* abnrm,
                           float* rconde, float* rcondv);

lapack_int LAPACKE_dgesv(int matrix_layout, lapack_int n, lapack_int nrhs,
                          double* a, lapack_int lda, lapack_int* ipiv,
                          double* b, lapack_int ldb);

lapack_int LAPACKE_sgesv(int matrix_layout, lapack_int n, lapack_int nrhs,
                          float* a, lapack_int lda, lapack_int* ipiv,
                          float* b, lapack_int ldb);

lapack_int LAPACKE_dsyevr(int matrix_layout, char jobz, char range, char uplo,
                           lapack_int n, double* a, lapack_int lda,
                           double vl, double vu, lapack_int il, lapack_int iu,
                           double abstol, lapack_int* m, double* w, double* z,
                           lapack_int ldz, lapack_int* isuppz);

lapack_int LAPACKE_ssyevr(int matrix_layout, char jobz, char range, char uplo,
                           lapack_int n, float* a, lapack_int lda,
                           float vl, float vu, lapack_int il, lapack_int iu,
                           float abstol, lapack_int* m, float* w, float* z,
                           lapack_int ldz, lapack_int* isuppz);

#ifdef __cplusplus
}
#endif

#endif
