
#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <queue>
#include <vector>
#include <cmath>
#include <iomanip>

double determinant(double **a, const size_t k);
void transpose(double **num, double **fac, double **inverse, const size_t r);
void cofactor(double **num, double **inverse, const size_t f);
void cofactor(double **num, double **inverse, const size_t f);
double **Make2DArray(const size_t rows, const size_t cols);
double **MatTrans(double **array, const size_t rows, const size_t cols);
double **MatMul(const size_t m1, const size_t m2, const size_t m3, double **A, double **B);
void MatVectMul(const size_t m1, const size_t m2, double **A, double *v, double *Av);
void PolyFit(std::queue<unsigned long> x, std::queue<double>y, const size_t n, const size_t k, const bool fixedinter,
const double fixedinterval, double *beta, double **Weights, double **XTWXInv);
double * queue_to_array(std::queue<double> q);
unsigned long * queue_to_array(std::queue<unsigned long> q);
double predictHumidity(std::queue<double> recordedHumidities, std::queue<unsigned long> recordedTimes, unsigned long t, size_t degree);


#endif