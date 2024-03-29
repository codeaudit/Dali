#ifndef DALI_TENSOR_OP_RESHAPING_H
#define DALI_TENSOR_OP_RESHAPING_H

#include "dali/tensor/Mat.h"
#include "dali/tensor/Tape.h"
#include "dali/utils.h"

template<typename R> class Mat;

namespace matops {
    template<typename R>
    struct Reshaping {
        static Mat<R> hstack(Mat<R>, Mat<R>);
        static Mat<R> hstack(std::initializer_list<Mat<R>>);
        static Mat<R> hstack(const std::vector<Mat<R>>&);
        static Mat<R> broadcast_row_vector(Mat<R> input, int num_rows);
        static Mat<R> broadcast_col_vector(Mat<R> input, int num_cols);
        static Mat<R> vstack(Mat<R>, Mat<R>);
        static Mat<R> vstack(std::initializer_list<Mat<R>>);
        static Mat<R> vstack(const std::vector<Mat<R>>&);
        static Mat<R> transpose(Mat<R>);
        static Mat<R> rows_pluck(Mat<R> matrix, Mat<int> indices);
        static Mat<R> rows_pluck(Mat<R>, Indexing::Index);
        static Mat<R> rows_cols_pluck(Mat<R>, Indexing::Index, Indexing::Index);
        static Mat<R> row_pluck(Mat<R>, int);
        static Mat<R> col_pluck(Mat<R>, int);
        static Mat<R> slice(Mat<R>, int, int);
        static void resize(Mat<R>& mat, dim_t rows, dim_t cols);
    };
}

#endif
