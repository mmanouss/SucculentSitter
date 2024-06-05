#include "algorithms.h"


// Calculates the determinant of a matrix 
// **************************************************************
double determinant(double **a, const size_t k) {
    double s = 1;
    double det = 0.;
    double **b = Make2DArray(k,k);
    size_t m;
    size_t n;

    if (k == 1) return (a[0][0]);
    for (size_t c=0; c<k; c++) {
        m = 0;
        n = 0;

        for (size_t i = 0; i < k; i++) {
            for (size_t j = 0; j < k; j++) {
                b[i][j] = 0;
                if (i != 0 && j != c) {
                    b[m][n] = a[i][j];
                    if (n < (k - 2)) {
                        n++;
                    }
                    else {
                        n = 0;
                        m++;
                    }
                }
            }
        }
        det = det + s * (a[0][c] * determinant(b, k - 1));
        s = -1 * s;
    }
    return (det);
}

// Perform the 
// **************************************************************
void transpose(double **num, double **fac, double **inverse, const size_t r) {
    double **b = Make2DArray(r,r);
    double deter;

    for (size_t i=0; i<r; i++) {
        for (size_t j=0; j<r; j++) {
            b[i][j] = fac[j][i];
        }
    }

    deter = determinant(num, r);

    for (size_t i=0; i<r; i++) {
        for (size_t j=0; j<r; j++) {
            inverse[i][j] = b[i][j] / deter;
        }
    }
}

// Calculates the cofactors 
// **************************************************************
void cofactor(double **num, double **inverse, const size_t f)
{
    double **b = Make2DArray(f,f);
    double **fac = Make2DArray(f,f);
   
    size_t m;
    size_t n;

    for (size_t q=0; q<f; q++) {
        for (size_t p=0; p<f; p++) {
            m = 0;
            n = 0;
            for (size_t i=0; i<f; i++) {
                for (size_t j=0; j<f; j++) {
                    if (i != q && j != p) {
                        b[m][n] = num[i][j];
                        if (n < (f - 2)) {
                            n++;
                        }
                        else {
                            n = 0;
                            m++;
                        }
                    }
                }
            }
            fac[q][p] = pow(-1, q + p) * determinant(b, f - 1);
        }
    }

    transpose(num, fac, inverse, f);
}

// Initialize a 2D array
// **************************************************************
double **Make2DArray(const size_t rows, const size_t cols) {

    double **array;

    array = new double*[rows];
    for(size_t i = 0; i < rows; i++) {
        array[i] = new double[cols];
    }

    for(size_t i = 0; i < rows; i++) {
        for(size_t j = 0; j < cols; j++) {
            array[i][j] = 0.;
        }
    }
    
    return array;

}

// Transpose a 2D array
// **************************************************************
double **MatTrans(double **array, const size_t rows, const size_t cols) {

    double **arrayT = Make2DArray(cols,rows);

    for(size_t i = 0; i < rows; i++) {
        for(size_t j = 0; j < cols; j++) {
            arrayT[j][i] = array[i][j];
        }
    }
    
    return arrayT;

}

// Perform the multiplication of matrix A[m1,m2] by B[m2,m3]
// **************************************************************
double **MatMul(const size_t m1, const size_t m2, const size_t m3, double **A, double **B) {

    double **array = Make2DArray(m1,m3);

    for (size_t i=0; i<m1; i++) {          
        for (size_t j=0; j<m3; j++) {      
            array[i][j]=0.; 
            for (size_t m=0; m<m2; m++) {
                array[i][j]+=A[i][m]*B[m][j];
            } 
        }       
    }
    return array;

}

// Perform the multiplication of matrix A[m1,m2] by vector v[m2,1]
// **************************************************************
void MatVectMul(const size_t m1, const size_t m2, double **A, double *v, double *Av) {

    
    for (size_t i=0; i<m1; i++) {   
        Av[i]=0.;
        for (size_t j=0; j<m2; j++) {
            Av[i]+=A[i][j]*v[j];    
        } 
    }
   

}

void PolyFit(time *x, double *y, const size_t n, const size_t k, const bool fixedinter,
const double fixedinterval, double *beta, double **Weights, double **XTWXInv) { 
  
    // Definition of variables
    // **************************************************************
    double **X = Make2DArray(n,k+1);           // [n,k+1]
    double **XT;                               // [k+1,n]
    double **XTW;                              // [k+1,n]
    double **XTWX;                             // [k+1,k+1]

    double *XTWY = new double[k+1];
    double *Y = new double[n];

    size_t begin = 0;
    if (fixedinter) begin = 1;

    // Initialize X
    // **************************************************************
    for (size_t i=0; i<n; i++) { 
        for (size_t j=begin; j<(k+1); j++) {  // begin
          X[i][j]=pow(x[i],j);  
        }       
    } 

    // Matrix calculations
    // **************************************************************
    XT = MatTrans(X, n, k+1);                 // Calculate XT
    XTW = MatMul(k+1,n,n,XT,Weights);         // Calculate XT*W
    XTWX = MatMul(k+1,n,k+1,XTW,X);           // Calculate (XTW)*X

    if (fixedinter) XTWX[0][0] = 1.;  
    
    cofactor(XTWX, XTWXInv, k+1);             // Calculate (XTWX)^-1

    for (size_t m=0; m<n; m++) {
        if (fixedinter) {
            Y[m]= y[m]-fixedinterval;
        } 
        else {
            Y[m] = y[m];
        }
    } 
    MatVectMul(k+1,n,XTW,Y,XTWY);             // Calculate (XTW)*Y
    MatVectMul(k+1,k+1,XTWXInv,XTWY,beta);    // Calculate beta = (XTWXInv)*XTWY

    if (fixedinter) beta[0] = fixedinterval;

    // cout << "Matrix X" << endl;
    // displayMat(X,n,k+1);

    // cout << "Matrix XT" << endl;
    // displayMat(XT,k+1,n);

    // cout << "Matrix XTW" << endl;
    // displayMat(XTW,k+1,n);

    // cout << "Matrix XTWXInv" << endl;
    // displayMat(XTWXInv,k+1,k+1);

}

double * queue_to_array(std::queue<double> q) {
    size_t q_size = q.size();
    double *array = new double [q_size];
    for (int i=0; i < q_size; i++){
        array[i] = q.front();
        q.pop();
    }
    return array;
}

time * queue_to_array(std::queue<time> q) {
    size_t q_size = q.size();
    time *array = new time [q_size];
    for (int i=0; i < q_size; i++){
        array[i] = q.front();
        q.pop();
    }
    return array;
}

double predictHumidity(std::queue<double> recordedHumidities, std::queue<time> recordedTimes, time t, size_t degree) {
    // takes last N recorded times produces a polynomial regression 
    double hypothesizedHumidity = 0.0;
    time millis();

    std::vector<double> coefficients;
    
    size_t n = recordedTimes.size(); // length of recordedTimes
    size_t k = degree;

    bool fixedinter = false; 
    double fixedinterval = 0.;                       // The fixed intercept value (if applicable)
    double coefbeta[k+1];                            // Coefficients of the polynomial
    double **XTWXInv = Make2DArray(k+1,k+1);
    double **Weights = Make2DArray(n,n);


    PolyFit(queue_to_array(recordedTimes), queue_to_array(recordedHumidities), n, k, fixedinter, fixedinterval, coefbeta, Weights, XTWXInv);

    int degree = coefficients.size(); // this is really degree-1
    for (int i=0; i < (int)degree + 1; i++){
        hypothesizedHumidity += coefficients[i] * (pow((double)t, i)); // calculate results of plugging in t value to poly function
    }

    recordedHumidities.pop(); // get rid of earliest humidity entry
    recordedHumidities.push(hypothesizedHumidity); // add latest humidity entry
    recordedTimes.pop(); // get rid of earliest time entry
    recordedTimes.push(millis()); // add latest time entry

    return hypothesizedHumidity;
}