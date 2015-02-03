#include "CrossEntropy.h"

template<typename T>
T cross_entropy(std::shared_ptr<Mat<T>> logprobs, int& target) {
	std::shared_ptr<Mat<T>> probs = softmax(logprobs);
	T cost = -std::log(probs->w(target,0));

	logprobs->dw = probs->w;
	// write gradients into log probabilities
	logprobs->dw(target, 0) -= 1;
	return cost;
}

template<typename T, typename M>
T cross_entropy(std::shared_ptr<Mat<T>> logprobs, const M targets) {
	std::shared_ptr<Mat<T>> probs = softmax(logprobs);
	T cost = 0.0;

	logprobs->dw = probs->w;
	// get cost for each pair of target and datastream:
	for (size_t i = 0; i < targets.rows(); i++) {
		cost -= std::log(probs->w(targets(i),i));
		logprobs->dw(targets(i), i) -= 1;
	}

	return cost;
}

template<typename Z, typename M, typename K, typename F>
Z masked_cross_entropy(std::shared_ptr<Mat<Z>> logprobs,
	uint& T,
	const K& loss_start,
	const F& codelens,
	const M targets) {
	std::shared_ptr<Mat<Z>> probs = softmax(logprobs);
	Z cost = 0.0;
	logprobs->dw = probs->w;
	// get cost for each pair of target and datastream:
	for (size_t i = 0; i < targets.rows(); i++) {
		if (T >= loss_start(i) && (T < loss_start(i) - codelens(i) )) {
			cost -= std::log(probs->w(targets(i),i));
			logprobs->dw.col(i) = probs->w.col(i);
			logprobs->dw(targets(i), i) -= 1;
		}
	}
	return cost;
}

template<typename Z, typename M, typename F>
Z masked_cross_entropy(std::shared_ptr<Mat<Z>> logprobs,
	uint& T,
	shared_eigen_index_vector loss_start,
	const F& codelens,
	const M targets) {
	std::shared_ptr<Mat<Z>> probs = softmax(logprobs);
	Z cost = 0.0;
	logprobs->dw = probs->w;
	// get cost for each pair of target and datastream:
	for (size_t i = 0; i < targets.rows(); i++) {
		if (T >= (*loss_start)(i) && (T < (*loss_start)(i) - codelens(i) )) {
			cost -= std::log(probs->w(targets(i),i));
			logprobs->dw.col(i) = probs->w.col(i);
			logprobs->dw(targets(i), i) -= 1;
		}
	}
	return cost;
}

template<typename Z, typename M, typename K>
Z masked_cross_entropy(std::shared_ptr<Mat<Z>> logprobs,
	uint& T,
	const K& loss_start,
	shared_eigen_index_vector codelens,
	const M targets) {
	std::shared_ptr<Mat<Z>> probs = softmax(logprobs);
	Z cost = 0.0;
	
	// get cost for each pair of target and datastream:
	for (size_t i = 0; i < targets.rows(); i++) {
		if (T >= loss_start(i) && (T < loss_start(i) - (*codelens)(i) )) {
			cost -= std::log(probs->w(targets(i),i));
			logprobs->dw.col(i) = probs->w.col(i);
			logprobs->dw(targets(i), i) -= 1;
		}
	}
	return cost;
}
/**
Masked Cross Entropy Loss
-------------------------

Given a probability distribution p at a time T, for k channels,
and k different targets, apply KL Divergence loss
on the channels that are have T >= loss_start[k] and
T < loss_start[k] + codelens[k] (so from T to T+codelen error
will be picked up by channel k).

Inputs
------

std::shared_ptr<Mat<Z>> logprobs : the log probabilities (unnormalized)
uint& T : the log probabilities (unnormalized)
shared_eigen_index_vector loss_start : where to start picking up errors for channel k
shared_eigen_index_vector codelens : how long does channel k pick up errors
const M targets : the labels at time T

Outputs
-------

Z cost : the total KL divergence at this time step for
         the relevant channels

*/
template<typename Z, typename M>
Z masked_cross_entropy(std::shared_ptr<Mat<Z>> logprobs,
	uint& T,
	shared_eigen_index_vector loss_start,
	shared_eigen_index_vector codelens,
	const M targets) {
	std::shared_ptr<Mat<Z>> probs = softmax(logprobs);
	Z cost = 0.0;
	// get cost for each pair of target and datastream:
	for (size_t i = 0; i < targets.rows(); i++) {
		if (T >= (*loss_start)(i) && (T < (*loss_start)(i) + (*codelens)(i) )) {
			cost -= std::log(probs->w(targets(i),i));
			logprobs->dw.col(i) = probs->w.col(i);
			logprobs->dw(targets(i), i) -= 1;
			// std::cout << "-- (" << i << ")\n";
			// for (int k = 0 ; k < logprobs->dw.rows(); k++) {
			// 	for (int j = 0; j < logprobs->dw.cols(); j++) {
			// 		if (k == targets(i) && i == j) {
			// 			std::cout << " **" << std::fixed
			// 			          << std::setw( 5 ) // keep 7 digits
			// 			          << std::setprecision( 3 ) // use 3 decimals
			// 			          << std::setfill( ' ' ) // pad values with blanks this->w(i,j)
			// 			          << logprobs->dw(k,j) << "** ";
			// 		} else {
			// 			std::cout << "   " << std::fixed
			// 			          << std::setw( 5 ) // keep 7 digits
			// 			          << std::setprecision( 3 ) // use 3 decimals
			// 			          << std::setfill( ' ' ) // pad values with blanks this->w(i,j)
			// 			          << logprobs->dw(k,j) << "   ";
			// 		}	
			// 	}
			// 	std::cout << "\n";
			// }
			// std::cout << std::endl << "--" << std::endl;
		}
	}
	return cost;
}

template float  cross_entropy(std::shared_ptr<Mat<float>>, eigen_index_block);
template double cross_entropy(std::shared_ptr<Mat<double>>, eigen_index_block);
template float  cross_entropy(std::shared_ptr<Mat<float>>, eigen_index_block_scalar);
template double cross_entropy(std::shared_ptr<Mat<double>>, eigen_index_block_scalar);

template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, const eigen_index_block&, const eigen_index_block&, const eigen_index_block);
template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, const eigen_index_vector&, const eigen_index_vector&, const eigen_index_block);
template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, const eigen_index_block&, const eigen_index_vector&, const eigen_index_block);
template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, const eigen_index_vector&, const eigen_index_block&, const eigen_index_block);
template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, shared_eigen_index_vector, const eigen_index_block&, const eigen_index_block);
template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, const eigen_index_vector&, shared_eigen_index_vector, const eigen_index_block);
template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, shared_eigen_index_vector, shared_eigen_index_vector, const eigen_index_block);

template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, const eigen_index_block&, const eigen_index_block&, const eigen_index_block);
template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, const eigen_index_vector&, const eigen_index_vector&, const eigen_index_block);
template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, const eigen_index_block&, const eigen_index_vector&, const eigen_index_block);
template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, const eigen_index_vector&, const eigen_index_block&, const eigen_index_block);
template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, shared_eigen_index_vector, const eigen_index_block&, const eigen_index_block);
template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, const eigen_index_vector&, shared_eigen_index_vector, const eigen_index_block);
template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, shared_eigen_index_vector, shared_eigen_index_vector, const eigen_index_block);

template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, const eigen_index_block&, const eigen_index_block&, const eigen_index_block_scalar);
template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, const eigen_index_vector&, const eigen_index_vector&, const eigen_index_block_scalar);
template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, const eigen_index_block&, const eigen_index_vector&, const eigen_index_block_scalar);
template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, const eigen_index_vector&, const eigen_index_block&, const eigen_index_block_scalar);
template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, shared_eigen_index_vector, const eigen_index_block&, const eigen_index_block_scalar);
template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, const eigen_index_vector&, shared_eigen_index_vector, const eigen_index_block_scalar);
template double masked_cross_entropy(std::shared_ptr<Mat<double>>, uint&, shared_eigen_index_vector, shared_eigen_index_vector, const eigen_index_block_scalar);

template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, const eigen_index_block&, const eigen_index_block&, const eigen_index_block_scalar);
template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, const eigen_index_vector&, const eigen_index_vector&, const eigen_index_block_scalar);
template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, const eigen_index_block&, const eigen_index_vector&, const eigen_index_block_scalar);
template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, const eigen_index_vector&, const eigen_index_block&, const eigen_index_block_scalar);
template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, shared_eigen_index_vector, const eigen_index_block&, const eigen_index_block_scalar);
template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, const eigen_index_vector&, shared_eigen_index_vector, const eigen_index_block_scalar);
template float masked_cross_entropy(std::shared_ptr<Mat<float>>, uint&, shared_eigen_index_vector, shared_eigen_index_vector, const eigen_index_block_scalar);

/**
Masked Sum
----------

Sum values[k] if timestep T
if within [loss_start[k], loss_start[k] + codelens[k]),
gradient is columnwise vector of 1s.

Inputs:
-------

std::shared_ptr<Mat<Z>> values : the data columns subject to summing.
uint& T : the log probabilities (unnormalized)
shared_eigen_index_vector loss_start : where to start picking up errors for channel k
shared_eigen_index_vector codelens : how long does channel k pick up errors

Outputs:
--------

Z cost : the total sum along the non-masked columns of values.

*/
template<typename Z>
Z masked_sum(std::shared_ptr<Mat<Z>> values,
	uint& T,
	shared_eigen_index_vector loss_start,
	shared_eigen_index_vector codelens,
	Z gparent) {

	Z cost = 0.0;
	// get cost for each pair of target and datastream:
	for (size_t i = 0; i < values->d; i++) {
		if (T >= (*loss_start)(i) && (T < (*loss_start)(i) + (*codelens)(i) )) {
			cost += values->w.col(i).sum() * gparent;
			values->dw.col(i).array() += gparent;
		}
	}
	return cost;
}

template<typename Z, typename K, typename F>
Z masked_sum(std::shared_ptr<Mat<Z>> values,
	uint& T,
	const K& loss_start,
	const F& codelens,
	Z gparent) {
	Z cost = 0.0;
	// get cost for each pair of target and datastream:
	for (size_t i = 0; i < values->d; i++) {
		if (T >= loss_start(i) && (T < loss_start(i) + codelens(i) )) {
			cost += values->w.col(i).sum() * gparent;
			values->dw.col(i).array() += gparent;
		}
	}
	return cost;
}

template<typename Z, typename K>
Z masked_sum(std::shared_ptr<Mat<Z>> values,
	uint& T,
	const K& loss_start,
	shared_eigen_index_vector codelens,
	Z gparent) {
	Z cost = 0.0;
	// get cost for each pair of target and datastream:
	for (size_t i = 0; i < values->d; i++) {
		if (T >= loss_start(i) && (T < loss_start(i) + (*codelens)(i) )) {
			cost += values->w.col(i).sum() * gparent;
			values->dw.col(i).array() += gparent;
		}
	}
	return cost;
}

template<typename Z, typename F>
Z masked_sum(std::shared_ptr<Mat<Z>> values,
	uint& T,
	shared_eigen_index_vector loss_start,
	const F& codelens,
	Z gparent) {
	Z cost = 0.0;
	// get cost for each pair of target and datastream:
	for (size_t i = 0; i < values->d; i++) {
		if (T >= (*loss_start)(i) && (T < (*loss_start)(i) + codelens(i) )) {
			cost += values->w.col(i).sum() * gparent;
			values->dw.col(i).array() += gparent;
		}
	}
	return cost;
}

template<typename Z>
Z masked_sum(std::shared_ptr<Mat<Z>> values,
	uint& T,
	int loss_start,
	shared_eigen_index_vector codelens,
	Z gparent) {
	Z cost = 0.0;
	// get cost for each pair of target and datastream:
	for (size_t i = 0; i < values->d; i++) {
		if (T >= loss_start && (T < loss_start + (*codelens)(i) )) {
			cost += values->w.col(i).sum() * gparent;
			values->dw.col(i).array() += gparent;
		}
	}
	return cost;
}

template<typename Z, typename F>
Z masked_sum(std::shared_ptr<Mat<Z>> values,
	uint& T,
	int loss_start,
	const F& codelens,
	Z gparent) {
	Z cost = 0.0;
	// get cost for each pair of target and datastream:
	for (size_t i = 0; i < values->d; i++) {
		if (T >= loss_start && (T < loss_start + codelens(i) )) {
			cost += values->w.col(i).sum() * gparent;
			values->dw.col(i).array() += gparent;
		}
	}
	return cost;
}

template double masked_sum(std::shared_ptr<Mat<double>>, uint&, const eigen_index_block&, const eigen_index_block&, double);
template double masked_sum(std::shared_ptr<Mat<double>>, uint&, const eigen_index_vector&, const eigen_index_vector&, double);
template double masked_sum(std::shared_ptr<Mat<double>>, uint&, const eigen_index_block&, const eigen_index_vector&, double);
template double masked_sum(std::shared_ptr<Mat<double>>, uint&, const eigen_index_vector&, const eigen_index_block&, double);
template double masked_sum(std::shared_ptr<Mat<double>>, uint&, shared_eigen_index_vector, const eigen_index_block&, double);
template double masked_sum(std::shared_ptr<Mat<double>>, uint&, const eigen_index_vector&, shared_eigen_index_vector, double);
template double masked_sum(std::shared_ptr<Mat<double>>, uint&, shared_eigen_index_vector, shared_eigen_index_vector, double);
template double masked_sum(std::shared_ptr<Mat<double>>, uint&, int, shared_eigen_index_vector, double);
template double masked_sum(std::shared_ptr<Mat<double>>, uint&, int, const eigen_index_block&, double);
template double masked_sum(std::shared_ptr<Mat<double>>, uint&, int, const eigen_index_vector&, double);

template float masked_sum(std::shared_ptr<Mat<float>>, uint&, const eigen_index_block&, const eigen_index_block&, float);
template float masked_sum(std::shared_ptr<Mat<float>>, uint&, const eigen_index_vector&, const eigen_index_vector&, float);
template float masked_sum(std::shared_ptr<Mat<float>>, uint&, const eigen_index_block&, const eigen_index_vector&, float);
template float masked_sum(std::shared_ptr<Mat<float>>, uint&, const eigen_index_vector&, const eigen_index_block&, float);
template float masked_sum(std::shared_ptr<Mat<float>>, uint&, shared_eigen_index_vector, const eigen_index_block&, float);
template float masked_sum(std::shared_ptr<Mat<float>>, uint&, const eigen_index_vector&, shared_eigen_index_vector, float);
template float masked_sum(std::shared_ptr<Mat<float>>, uint&, shared_eigen_index_vector, shared_eigen_index_vector, float);
template float masked_sum(std::shared_ptr<Mat<float>>, uint&, int, shared_eigen_index_vector, float);
template float masked_sum(std::shared_ptr<Mat<float>>, uint&, int, const eigen_index_block&, float);
template float masked_sum(std::shared_ptr<Mat<float>>, uint&, int, const eigen_index_vector&, float);

template float cross_entropy(std::shared_ptr<Mat<float>>, int&);
template double cross_entropy(std::shared_ptr<Mat<double>>, int&);
