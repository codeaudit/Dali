#include "Softmax.h"

template<typename T>
std::shared_ptr<Mat<T>> softmax_transpose(std::shared_ptr<Mat<T>> matrix) {
	auto layer_max = matrix->w.rowwise().maxCoeff().array().matrix();
	auto exped_distributions = (matrix->w.colwise() - layer_max.col(0)).array().exp().matrix();
	
	auto out = std::make_shared<Mat<T>>(
		matrix->n,
		matrix->d,
		false);

	auto total_distribution = exped_distributions.rowwise().sum().array().matrix();
	out->w = (exped_distributions.array().colwise() / total_distribution.col(0).array());
	return out;
}

template<typename T>
std::shared_ptr<Mat<T>> softmax(std::shared_ptr<Mat<T>> matrix) {
	auto layer_max = matrix->w.colwise().maxCoeff().array().matrix();
	auto exped_distributions = (matrix->w.rowwise() - layer_max.row(0)).array().exp().matrix();
	
	auto out = std::make_shared<Mat<T>>(
		matrix->n,
		matrix->d,
		false);

	auto total_distribution = exped_distributions.colwise().sum().array().matrix();
	out->w = (exped_distributions.array().rowwise() / total_distribution.row(0).array());
	return out;
}

template std::shared_ptr<Mat<float>> softmax(std::shared_ptr<Mat<float>>);
template std::shared_ptr<Mat<double>> softmax(std::shared_ptr<Mat<double>>);

template std::shared_ptr<Mat<float>> softmax_transpose(std::shared_ptr<Mat<float>>);
template std::shared_ptr<Mat<double>> softmax_transpose(std::shared_ptr<Mat<double>>);