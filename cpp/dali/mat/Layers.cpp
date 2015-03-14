#include "Layers.h"

using std::make_shared;
using std::vector;
using std::make_tuple;
using std::get;

typedef std::pair<int,int> PII;

template<typename R>
Mat<R> AbstractMultiInputLayer<R>::activate(const std::vector<Mat<R>>& inputs) const {
    assert(inputs.size() > 0);
    return activate(inputs[0]);
};

template<typename R>
Mat<R> AbstractMultiInputLayer<R>::activate(Mat<R> first_input, const std::vector<Mat<R>>& inputs) const {
    if (inputs.size() > 0) {
        return activate(inputs.back());
    } else {
        return activate(first_input);
    }
};

template<typename R>
void Layer<R>::create_variables() {
    R upper = 1. / sqrt(input_size);
    W = Mat<R>(hidden_size, input_size, -upper, upper);
    this->b = Mat<R>(hidden_size, 1);
}
template<typename R>
Layer<R>::Layer (int _input_size, int _hidden_size) : hidden_size(_hidden_size), input_size(_input_size) {
    create_variables();
}

template<typename R>
Mat<R> Layer<R>::activate(Mat<R> input_vector) const {
    return MatOps<R>::mul_with_bias(W, input_vector, this->b);
}

template<typename R>
Layer<R>::Layer (const Layer<R>& layer, bool copy_w, bool copy_dw) : hidden_size(layer.hidden_size), input_size(layer.input_size) {
    W = Mat<R>(layer.W, copy_w, copy_dw);
    this->b = Mat<R>(layer.b, copy_w, copy_dw);
}

template<typename R>
Layer<R> Layer<R>::shallow_copy() const {
    return Layer<R>(*this, false, true);
}

template<typename R>
std::vector<Mat<R>> Layer<R>::parameters() const{
    return std::vector<Mat<R>>({W, this->b});
}

// StackedInputLayer:
template<typename R>
void StackedInputLayer<R>::create_variables() {
    int total_input_size = 0;
    for (auto& input_size : input_sizes) total_input_size += input_size;
    R upper = 1. / sqrt(total_input_size);
    matrices.reserve(input_sizes.size());
    for (auto& input_size : input_sizes) {
        matrices.emplace_back(hidden_size, input_size, -upper, upper);
        DEBUG_ASSERT_MAT_NOT_NAN(matrices[matrices.size() -1]);
    }
    this->b = Mat<R>(hidden_size, 1);
}
template<typename R>
StackedInputLayer<R>::StackedInputLayer (vector<int> _input_sizes,
                                         int _hidden_size) :
        hidden_size(_hidden_size),
        input_sizes(_input_sizes) {
    create_variables();
}

template<typename R>
StackedInputLayer<R>::StackedInputLayer (std::initializer_list<int> _input_sizes,
                                         int _hidden_size) :
        hidden_size(_hidden_size),
        input_sizes(_input_sizes) {
    create_variables();
}

template<typename R>
vector<Mat<R>> StackedInputLayer<R>::zip_inputs_with_matrices_and_bias(const vector<Mat<R>>& inputs) const {
    vector<Mat<R>> zipped;
    zipped.reserve(matrices.size() * 2 + 1);
    auto input_ptr = inputs.begin();
    auto mat_ptr = matrices.begin();
    while (mat_ptr != matrices.end()) {
        zipped.emplace_back(*mat_ptr++);
        zipped.emplace_back(*input_ptr++);
    }
    zipped.emplace_back(this->b);
    return zipped;
}

template<typename R>
vector<Mat<R>> StackedInputLayer<R>::zip_inputs_with_matrices_and_bias(
        Mat<R> input,
        const vector<Mat<R>>& inputs) const {
    vector<Mat<R>> zipped;
    zipped.reserve(matrices.size() * 2 + 1);
    auto input_ptr = inputs.begin();
    auto mat_ptr = matrices.begin();

    // We are provided separately with anoter input vector
    // that will go first in the zip, while the remainder will
    // be loaded in "zip" form with the vector of inputs
    zipped.emplace_back(*mat_ptr);
    zipped.emplace_back(input);

    mat_ptr++;

    DEBUG_ASSERT_MAT_NOT_NAN((*mat_ptr));
    DEBUG_ASSERT_MAT_NOT_NAN(input);

    while (mat_ptr != matrices.end()) {

        DEBUG_ASSERT_MAT_NOT_NAN((*mat_ptr));
        DEBUG_ASSERT_MAT_NOT_NAN((*input_ptr));

        zipped.emplace_back(*mat_ptr);
        zipped.emplace_back(*input_ptr);
        mat_ptr++;
        input_ptr++;
    }
    zipped.emplace_back(this->b);
    return zipped;
}

template<typename R>
Mat<R> StackedInputLayer<R>::activate(
    const vector<Mat<R>>& inputs) const {
    auto zipped = zip_inputs_with_matrices_and_bias(inputs);
    return MatOps<R>::mul_add_mul_with_bias(zipped);
}

template<typename R>
Mat<R> StackedInputLayer<R>::activate(
    Mat<R> input_vector) const {
    if (matrices.size() == 0) {
        return MatOps<R>::mul_with_bias(matrices[0], input_vector, this->b);
    } else {
        throw std::runtime_error("Error: Stacked Input Layer parametrized with more than 1 inputs only received 1 input vector.");
    }
}

template<typename R>
Mat<R> StackedInputLayer<R>::activate(
    Mat<R> input,
    const vector<Mat<R>>& inputs) const {
    DEBUG_ASSERT_MAT_NOT_NAN(input);
    auto zipped = zip_inputs_with_matrices_and_bias(input, inputs);

    auto out = MatOps<R>::mul_add_mul_with_bias(zipped);

    DEBUG_ASSERT_MAT_NOT_NAN(out);

    return out;
}

template<typename R>
StackedInputLayer<R>::StackedInputLayer (const StackedInputLayer<R>& layer, bool copy_w, bool copy_dw) : hidden_size(layer.hidden_size), input_sizes(layer.input_sizes) {
    matrices.reserve(layer.matrices.size());
    for (auto& matrix : layer.matrices)
        matrices.emplace_back(matrix, copy_w, copy_dw);
    this->b = Mat<R>(layer.b, copy_w, copy_dw);
}

template<typename R>
StackedInputLayer<R> StackedInputLayer<R>::shallow_copy() const {
    return StackedInputLayer<R>(*this, false, true);
}

template<typename R>
std::vector<Mat<R>> StackedInputLayer<R>::parameters() const{
    auto params = vector<Mat<R>>(matrices);
    params.emplace_back(this->b);
    return params;
}

template<typename R>
void RNN<R>::create_variables() {
    R upper = 1. / sqrt(input_size);
    Wx = Mat<R>(output_size, input_size,  -upper, upper);
    upper = 1. / sqrt(hidden_size);

    Wh = Mat<R>(output_size, hidden_size, -upper, upper);
    b  = Mat<R>(output_size, 1, -upper, upper);
}

template<typename R>
void ShortcutRNN<R>::create_variables() {
    R upper = 1. / sqrt(input_size);
    Wx = Mat<R>(output_size, input_size,  -upper, upper);
    upper = 1. / sqrt(shortcut_size);
    Ws = Mat<R>(output_size, shortcut_size,  -upper, upper);
    upper = 1. / sqrt(hidden_size);
    Wh = Mat<R>(output_size, hidden_size, -upper, upper);
    b  = Mat<R>(output_size, 1, -upper, upper);
}


template<typename R>
std::vector<Mat<R>> RNN<R>::parameters() const {
    return std::vector<Mat<R>>({Wx, Wh, b});
}

/* DelayedRNN */

template<typename R>
DelayedRNN<R>::DelayedRNN(int input_size, int hidden_size, int output_size) :
        hidden_rnn(input_size, hidden_size),
        output_rnn(input_size, hidden_size, output_size) {
}

template<typename R>
DelayedRNN<R>::DelayedRNN (const DelayedRNN<R>& rnn, bool copy_w, bool copy_dw) :
        hidden_rnn(rnn.hidden_rnn, copy_w, copy_dw),
        output_rnn(rnn.output_rnn, copy_w, copy_dw) {
}


template<typename R>
std::vector<Mat<R>> DelayedRNN<R>::parameters() const {
    std::vector<Mat<R>> ret;
    for (auto& model: {hidden_rnn, output_rnn}) {
        auto params = model.parameters();
        ret.insert(ret.end(), params.begin(), params.end());
    }
    return ret;
}

template<typename R>
Mat<R> DelayedRNN<R>::initial_states() const {
    return Mat<R>(hidden_rnn.hidden_size, 1);
}

template<typename R>
std::tuple<Mat<R>,Mat<R>> DelayedRNN<R>::activate(
        Mat<R> input_vector,
        Mat<R> prev_hidden) const {

    return std::make_tuple(
        hidden_rnn.activate(input_vector, prev_hidden),
        output_rnn.activate(input_vector, prev_hidden)
    );
}

template<typename R>
DelayedRNN<R> DelayedRNN<R>::shallow_copy() const {
    return DelayedRNN<R>(*this, false, true);
}

template class DelayedRNN<float>;
template class DelayedRNN<double>;


/* StackedInputLayer */


template<typename R>
std::vector<Mat<R>> ShortcutRNN<R>::parameters() const {
    return std::vector<Mat<R>>({Wx, Wh, Ws, b});
}

template<typename R>
RNN<R>::RNN (int _input_size, int _hidden_size) :
        hidden_size(_hidden_size),
        input_size(_input_size),
        output_size(_hidden_size) {
    create_variables();
}

template<typename R>
RNN<R>::RNN (int _input_size, int _hidden_size, int _output_size) :\
        hidden_size(_hidden_size),
        input_size(_input_size),
        output_size(_output_size) {
    create_variables();
}

template<typename R>
ShortcutRNN<R>::ShortcutRNN (int _input_size, int _shortcut_size, int _hidden_size) :
        hidden_size(_hidden_size),
        input_size(_input_size),
        output_size(_hidden_size),
        shortcut_size(_shortcut_size) {
    create_variables();
}

template<typename R>
ShortcutRNN<R>::ShortcutRNN (int _input_size,
                             int _shortcut_size,
                             int _hidden_size,
                             int _output_size) :
        hidden_size(_hidden_size),
        input_size(_input_size),
        shortcut_size(_shortcut_size),
        output_size(_output_size) {
    create_variables();
}

template<typename R>
RNN<R>::RNN (const RNN<R>& rnn, bool copy_w, bool copy_dw) :
        hidden_size(rnn.hidden_size),
        input_size(rnn.input_size),
        output_size(rnn.output_size) {
    Wx = Mat<R>(rnn.Wx, copy_w, copy_dw);
    Wh = Mat<R>(rnn.Wh, copy_w, copy_dw);
    b = Mat<R>(rnn.b, copy_w, copy_dw);
}

template<typename R>
ShortcutRNN<R>::ShortcutRNN (const ShortcutRNN<R>& rnn,
                             bool copy_w,
                             bool copy_dw) :
        hidden_size(rnn.hidden_size),
        input_size(rnn.input_size),
        output_size(rnn.output_size),
        shortcut_size(rnn.shortcut_size) {
    Wx = Mat<R>(rnn.Wx, copy_w, copy_dw);
    Wh = Mat<R>(rnn.Wh, copy_w, copy_dw);
    Ws = Mat<R>(rnn.Ws, copy_w, copy_dw);
    b = Mat<R>(rnn.b, copy_w, copy_dw);
}

template<typename R>
RNN<R> RNN<R>::shallow_copy() const {
    return RNN<R>(*this, false, true);
}

template<typename R>
ShortcutRNN<R> ShortcutRNN<R>::shallow_copy() const {
    return ShortcutRNN<R>(*this, false, true);
}

template<typename R>
Mat<R> RNN<R>::activate(
    Mat<R> input_vector,
    Mat<R> prev_hidden) const {
    // takes 5% less time to run operations when grouping them (no big gains then)
    // 1.118s with explicit (& temporaries) vs 1.020s with grouped expression & backprop
    // return G.add(G.mul(Wx, input_vector), G.mul_with_bias(Wh, prev_hidden, b));
    DEBUG_ASSERT_NOT_NAN(Wx.w());
    DEBUG_ASSERT_NOT_NAN(input_vector.w());
    DEBUG_ASSERT_NOT_NAN(Wh.w());
    DEBUG_ASSERT_NOT_NAN(prev_hidden.w());
    DEBUG_ASSERT_NOT_NAN(b.w());
    return MatOps<R>::mul_add_mul_with_bias(Wx, input_vector, Wh, prev_hidden, b);
}

template<typename R>
Mat<R> ShortcutRNN<R>::activate(
    Mat<R> input_vector,
    Mat<R> shortcut_vector,
    Mat<R> prev_hidden) const {
    // takes 5% less time to run operations when grouping them (no big gains then)
    // 1.118s with explicit (& temporaries) vs 1.020s with grouped expression & backprop
    // return G.add(G.mul(Wx, input_vector), G.mul_with_bias(Wh, prev_hidden, b));
    DEBUG_ASSERT_NOT_NAN(Wx.w());
    DEBUG_ASSERT_NOT_NAN(input_vector.w());
    DEBUG_ASSERT_NOT_NAN(Wh.w());
    DEBUG_ASSERT_NOT_NAN(prev_hidden.w());
    DEBUG_ASSERT_NOT_NAN(b.w());
    return Ws.mul(shortcut_vector).add(MatOps<R>::mul_add_mul_with_bias(Wx, input_vector, Wh, prev_hidden, b));
}

template<typename R>
GatedInput<R>::GatedInput (int _input_size, int _hidden_size) :
        RNN<R>(_input_size, _hidden_size, 1) {
}

template<typename R>
GatedInput<R>::GatedInput (const GatedInput<R>& gate,
                           bool copy_w,
                           bool copy_dw) :
        RNN<R>(gate, copy_w, copy_dw) {
}

template<typename R>
GatedInput<R> GatedInput<R>::shallow_copy() const {
    return GatedInput<R>(*this, false, true);
}

template<typename R>
void LSTM<R>::name_internal_layers() {
    forget_layer.b.set_name("LSTM Forget bias");
    forget_layer.Wx.set_name("LSTM Forget Wx");
    forget_layer.Wh.set_name("LSTM Forget Wh");

    input_layer.b.set_name("LSTM Input bias");
    input_layer.Wx.set_name("LSTM Input Wx");
    input_layer.Wh.set_name("LSTM Input Wh");

    output_layer.b.set_name("LSTM Output bias");
    output_layer.Wx.set_name("LSTM Output Wx");
    output_layer.Wh.set_name("LSTM Output Wh");

    cell_layer.b.set_name("LSTM Cell bias");
    cell_layer.Wx.set_name("LSTM Cell Wx");
    cell_layer.Wh.set_name("LSTM Cell Wh");
}

template<typename R>
void ShortcutLSTM<R>::name_internal_layers() {
    forget_layer.b.set_name("Shortcut Forget bias");
    forget_layer.Wx.set_name("Shortcut Forget Wx");
    forget_layer.Wh.set_name("Shortcut Forget Wh");
    forget_layer.Ws.set_name("Shortcut Forget Ws");

    input_layer.b.set_name("Shortcut Input bias");
    input_layer.Wx.set_name("Shortcut Input Wx");
    input_layer.Wh.set_name("Shortcut Input Wh");
    input_layer.Ws.set_name("Shortcut Input Ws");

    output_layer.b.set_name("Shortcut Output bias");
    output_layer.Wx.set_name("Shortcut Output Wx");
    output_layer.Wh.set_name("Shortcut Output Wh");
    output_layer.Ws.set_name("Shortcut Output Ws");

    cell_layer.b.set_name("Shortcut Cell bias");
    cell_layer.Wx.set_name("Shortcut Cell Wx");
    cell_layer.Wh.set_name("Shortcut Cell Wh");
    cell_layer.Ws.set_name("Shortcut Cell Ws");
}

template<typename R>
LSTM<R>::LSTM (int _input_size, int _hidden_size) :
        hidden_size(_hidden_size),
        input_size(_input_size),
        input_layer(_input_size, _hidden_size),
        forget_layer(_input_size, _hidden_size),
        output_layer(_input_size, _hidden_size),
        cell_layer(_input_size, _hidden_size) {
    // Note: Ilya Sutskever recommends initializing with
    // forget gate at high value
    // http://yyue.blogspot.fr/2015/01/a-brief-overview-of-deep-learning.html
    // forget_layer.b.w().array() += 2;
    name_internal_layers();
}

template<typename R>
ShortcutLSTM<R>::ShortcutLSTM (int _input_size, int _shortcut_size, int _hidden_size) :
        hidden_size(_hidden_size),
        input_size(_input_size),
        shortcut_size(_shortcut_size),
        input_layer(_input_size, _shortcut_size, _hidden_size),
        forget_layer(_input_size, _shortcut_size, _hidden_size),
        output_layer(_input_size, _shortcut_size, _hidden_size),
        cell_layer(_input_size, _shortcut_size, _hidden_size) {
    // forget_layer.b.w().array() += 2;
    name_internal_layers();
}

template<typename R>
LSTM<R>::LSTM (const LSTM<R>& lstm, bool copy_w, bool copy_dw) :
        hidden_size(lstm.hidden_size),
        input_size(lstm.input_size),
        input_layer(lstm.input_layer, copy_w, copy_dw),
        forget_layer(lstm.forget_layer, copy_w, copy_dw),
        output_layer(lstm.output_layer, copy_w, copy_dw),
        cell_layer(lstm.cell_layer, copy_w, copy_dw) {
    name_internal_layers();
}

template<typename R>
ShortcutLSTM<R>::ShortcutLSTM (const ShortcutLSTM<R>& lstm, bool copy_w, bool copy_dw) :
        hidden_size(lstm.hidden_size),
        input_size(lstm.input_size),
        shortcut_size(lstm.shortcut_size),
        input_layer(lstm.input_layer, copy_w, copy_dw),
        forget_layer(lstm.forget_layer, copy_w, copy_dw),
        output_layer(lstm.output_layer, copy_w, copy_dw),
        cell_layer(lstm.cell_layer, copy_w, copy_dw) {
    name_internal_layers();
}

template<typename R>
LSTM<R> LSTM<R>::shallow_copy() const {
    return LSTM<R>(*this, false, true);
}
template<typename R>
ShortcutLSTM<R> ShortcutLSTM<R>::shallow_copy() const {
    return ShortcutLSTM<R>(*this, false, true);
}

template<typename R>
std::tuple<Mat<R>, Mat<R>> LSTM<R>::activate(
    Mat<R> input_vector,
    Mat<R> cell_prev,
    Mat<R> hidden_prev) const {

    // input gate:
    auto input_gate  = input_layer.activate(input_vector, hidden_prev).sigmoid();
    // forget gate
    auto forget_gate = forget_layer.activate(input_vector, hidden_prev).sigmoid();
    // output gate
    auto output_gate = output_layer.activate(input_vector, hidden_prev).sigmoid();
    // write operation on cells
    auto cell_write  = cell_layer.activate(input_vector, hidden_prev).tanh();

    // compute new cell activation
    auto retain_cell = forget_gate.eltmul(cell_prev); // what do we keep from cell
    auto write_cell  = input_gate.eltmul(cell_write); // what do we write to cell
    auto cell_d      = retain_cell.add(write_cell); // new cell contents

    // compute hidden state as gated, saturated cell activations

    auto hidden_d    = output_gate.eltmul(cell_d.tanh());

    DEBUG_ASSERT_NOT_NAN(hidden_d.w());
    DEBUG_ASSERT_NOT_NAN(cell_d.w());

    return make_tuple(cell_d, hidden_d);
}

template<typename R>
typename LSTM<R>::state_t LSTM<R>::activate_sequence(
    state_t initial_state,
    const Seq<Mat<R>>& sequence) const {
    for (auto& input_vector : sequence)
        initial_state = activate(
            input_vector,
            std::get<0>(initial_state),
            std::get<1>(initial_state)
        );
    return initial_state;
};

template<typename R>
std::tuple<Mat<R>, Mat<R>> ShortcutLSTM<R>::activate (
    Mat<R> input_vector,
    Mat<R> shortcut_vector,
    Mat<R> cell_prev,
    Mat<R> hidden_prev) const {

    // input gate:
    auto input_gate  = input_layer.activate(input_vector, shortcut_vector, hidden_prev).sigmoid();
    // forget gate
    auto forget_gate = forget_layer.activate(input_vector, shortcut_vector, hidden_prev).sigmoid();
    // output gate
    auto output_gate = output_layer.activate(input_vector, shortcut_vector, hidden_prev).sigmoid();
    // write operation on cells
    auto cell_write  = cell_layer.activate(input_vector, shortcut_vector, hidden_prev).tanh();

    // compute new cell activation
    auto retain_cell = forget_gate.eltmul(cell_prev); // what do we keep from cell
    auto write_cell  = input_gate.eltmul(cell_write); // what do we write to cell
    auto cell_d      = retain_cell.add(write_cell); // new cell contents

    // compute hidden state as gated, saturated cell activations

    auto hidden_d    = output_gate.eltmul(cell_d.tanh());

    DEBUG_ASSERT_NOT_NAN(hidden_d.w());
    DEBUG_ASSERT_NOT_NAN(cell_d.w());

    return make_tuple(cell_d, hidden_d);
}

template<typename R>
typename ShortcutLSTM<R>::state_t ShortcutLSTM<R>::activate_sequence(
    state_t initial_state,
    const Seq<Mat<R>>& sequence,
    const Seq<Mat<R>>& shortcut_sequence) const {
    assert(sequence.size() == shortcut_sequence.size());
    auto seq_begin = shortcut_sequence.begin();
    for (auto& input_vector : sequence) {
        initial_state = activate(
            input_vector,
            *seq_begin,
            std::get<0>(initial_state),
            std::get<1>(initial_state)
        );
        seq_begin++;
    }
    return initial_state;
};

template<typename R>
std::vector<Mat<R>> LSTM<R>::parameters() const {
    std::vector<Mat<R>> parameters;

    auto input_layer_params  = input_layer.parameters();
    auto forget_layer_params = forget_layer.parameters();
    auto output_layer_params = output_layer.parameters();
    auto cell_layer_params   = cell_layer.parameters();

    parameters.insert( parameters.end(), input_layer_params.begin(),  input_layer_params.end() );
    parameters.insert( parameters.end(), forget_layer_params.begin(), forget_layer_params.end() );
    parameters.insert( parameters.end(), output_layer_params.begin(), output_layer_params.end() );
    parameters.insert( parameters.end(), cell_layer_params.begin(),   cell_layer_params.end() );

    return parameters;
}

template<typename R>
std::vector<Mat<R>> ShortcutLSTM<R>::parameters() const {
    std::vector<Mat<R>> parameters;

    auto input_layer_params  = input_layer.parameters();
    auto forget_layer_params = forget_layer.parameters();
    auto output_layer_params = output_layer.parameters();
    auto cell_layer_params   = cell_layer.parameters();

    parameters.insert( parameters.end(), input_layer_params.begin(),  input_layer_params.end() );
    parameters.insert( parameters.end(), forget_layer_params.begin(), forget_layer_params.end() );
    parameters.insert( parameters.end(), output_layer_params.begin(), output_layer_params.end() );
    parameters.insert( parameters.end(), cell_layer_params.begin(),   cell_layer_params.end() );

    return parameters;
}

template<typename R>
typename LSTM<R>::state_t LSTM<R>::initial_states() const {
    return state_t(Mat<R>(hidden_size, 1), Mat<R>(hidden_size, 1));
}
template<typename R>
typename ShortcutLSTM<R>::state_t ShortcutLSTM<R>::initial_states() const {
    return state_t(Mat<R>(hidden_size, 1), Mat<R>(hidden_size, 1));
}

template<typename R>
std::tuple< std::vector<Mat<R>>, std::vector<Mat<R>> > LSTM<R>::initial_states(
        const std::vector<int>& hidden_sizes) {
    std::tuple< std::vector<Mat<R>>, std::vector<Mat<R>> > initial_state;
    get<0>(initial_state).reserve(hidden_sizes.size());
    std::get<1>(initial_state).reserve(hidden_sizes.size());
    for (auto& size : hidden_sizes) {
        get<0>(initial_state).emplace_back(Mat<R>(size, 1));
        std::get<1>(initial_state).emplace_back(Mat<R>(size, 1));
    }
    return initial_state;
}

using std::vector;
using std::shared_ptr;

template<typename celltype>
vector<celltype> StackedCells(const int& input_size, const vector<int>& hidden_sizes) {
    vector<celltype> cells;
    cells.reserve(hidden_sizes.size());
    int prev_size = input_size;
    for (auto& size : hidden_sizes) {
        cells.emplace_back(prev_size, size);
        prev_size = size;
    }
    return cells;
}

template <typename celltype>
vector<celltype> StackedCells(const int& input_size,
                              const int& shortcut_size,
                              const vector<int>& hidden_sizes) {
    vector<celltype> cells;
    cells.reserve(hidden_sizes.size());
    int prev_size = input_size;
    for (auto& size : hidden_sizes) {
        cells.emplace_back(prev_size, size);
        prev_size = size;
    }
    return cells;
}

template<> vector<ShortcutLSTM<float>> StackedCells<ShortcutLSTM<float>>(
        const int& input_size,
        const int& shortcut_size,
        const vector<int>& hidden_sizes) {
    vector<ShortcutLSTM<float>> cells;
    cells.reserve(hidden_sizes.size());
    int prev_size = input_size;
    for (auto& size : hidden_sizes) {
        cells.emplace_back(prev_size, shortcut_size, size);
        prev_size = size;
    }
    return cells;
}

template<> vector<ShortcutLSTM<double>> StackedCells<ShortcutLSTM<double>>(
        const int& input_size,
        const int& shortcut_size,
        const vector<int>& hidden_sizes) {
    vector<ShortcutLSTM<double>> cells;
    cells.reserve(hidden_sizes.size());
    int prev_size = input_size;
    for (auto& size : hidden_sizes) {
        cells.emplace_back(prev_size, shortcut_size, size);
        prev_size = size;
    }
    return cells;
}

template<typename celltype>
vector<celltype> StackedCells(const vector<celltype>& source_cells,
                              bool copy_w,
                              bool copy_dw) {
    vector<celltype> cells;
    cells.reserve(source_cells.size());
    for (const auto& cell : source_cells)
        cells.emplace_back(cell, copy_w, copy_dw);
    return cells;
}

template<typename R>
std::tuple<vector<Mat<R>>, vector<Mat<R>>> forward_LSTMs(
    Mat<R> input_vector,
    std::tuple<vector<Mat<R>>, vector<Mat<R>>>& previous_state,
    const vector<LSTM<R>>& cells,
    R drop_prob) {

    auto previous_state_cells = get<0>(previous_state);
    auto previous_state_hiddens = std::get<1>(previous_state);

    auto cell_iter = previous_state_cells.begin();
    auto hidden_iter = previous_state_hiddens.begin();

    std::tuple<vector<Mat<R>>, vector<Mat<R>>> out_state;
    get<0>(out_state).reserve(cells.size());
    get<1>(out_state).reserve(cells.size());

    auto layer_input = input_vector;

    for (auto& layer : cells) {

        auto layer_out = layer.activate(MatOps<R>::dropout_normalized(layer_input, drop_prob),
                                        *cell_iter,
                                        *hidden_iter);

        get<0>(out_state).push_back(get<0>(layer_out));
        get<1>(out_state).push_back(get<1>(layer_out));

        ++cell_iter;
        ++hidden_iter;

        layer_input = get<1>(layer_out);
    }

    return out_state;
}


template<typename R>
std::tuple<vector<Mat<R>>, vector<Mat<R>>> forward_LSTMs(
    Mat<R> input_vector,
    std::tuple<vector<Mat<R>>, vector<Mat<R>>>& previous_state,
    const LSTM<R>& base_cell,
    const vector<ShortcutLSTM<R>>& cells,
    R drop_prob) {

    auto previous_state_cells = get<0>(previous_state);
    auto previous_state_hiddens = std::get<1>(previous_state);

    auto cell_iter = previous_state_cells.begin();
    auto hidden_iter = previous_state_hiddens.begin();

    std::tuple<vector<Mat<R>>, vector<Mat<R>>> out_state;
    get<0>(out_state).reserve(cells.size() + 1);
    get<1>(out_state).reserve(cells.size() + 1);

    auto layer_input = input_vector;

    auto layer_out = base_cell.activate(layer_input, *cell_iter, *hidden_iter);
    get<0>(out_state).push_back(get<0>(layer_out));
    get<1>(out_state).push_back(get<1>(layer_out));

    ++cell_iter;
    ++hidden_iter;

    layer_input = get<1>(layer_out);

    for (auto& layer : cells) {

        // Rhe next cell up gets both the base input (input_vector)
        // and the cell below's input activation (layer_input)
        // => façon Alex Graves
        layer_out = layer.activate(MatOps<R>::dropout_normalized(layer_input, drop_prob),
                                   MatOps<R>::dropout_normalized(input_vector, drop_prob),
                                   *cell_iter,
                                   *hidden_iter);

        get<0>(out_state).push_back(get<0>(layer_out));
        get<1>(out_state).push_back(get<1>(layer_out));

        ++cell_iter;
        ++hidden_iter;

        layer_input = get<1>(layer_out);
    }

    return out_state;
}

template class Layer<float>;
template class Layer<double>;

template class StackedInputLayer<float>;
template class StackedInputLayer<double>;

template class RNN<float>;
template class RNN<double>;

template class ShortcutRNN<float>;
template class ShortcutRNN<double>;

template std::vector<RNN<float>> StackedCells <RNN<float>>(const int&, const std::vector<int>&);
template std::vector<RNN<double>> StackedCells <RNN<double>>(const int&, const std::vector<int>&);

template class GatedInput<float>;
template class GatedInput<double>;

template std::vector<GatedInput<float>> StackedCells <GatedInput<float>>(const int&, const std::vector<int>&);
template std::vector<GatedInput<double>> StackedCells <GatedInput<double>>(const int&, const std::vector<int>&);

template class LSTM<float>;
template class LSTM<double>;

template class ShortcutLSTM<float>;
template class ShortcutLSTM<double>;

template<typename R>
AbstractStackedLSTM<R>::AbstractStackedLSTM(const int& input_size, const std::vector<int>& hidden_sizes) :
        input_size(input_size),
        hidden_sizes(hidden_sizes) {
}

template<typename R>
AbstractStackedLSTM<R>::AbstractStackedLSTM(const AbstractStackedLSTM<R>& model, bool copy_w, bool copy_dw) :
        input_size(model.input_size),
        hidden_sizes(model.hidden_sizes) {
}

template<typename R>
typename AbstractStackedLSTM<R>::state_t AbstractStackedLSTM<R>::initial_states() const {
    return LSTM<R>::initial_states(hidden_sizes);
}


template<typename R>
typename AbstractStackedLSTM<R>::state_t AbstractStackedLSTM<R>::activate_sequence(
    state_t initial_state,
    const Seq<Mat<R>>& sequence,
    R drop_prob) const {
    for (auto& input_vector : sequence)
        initial_state = activate(initial_state, input_vector, drop_prob);
    return initial_state;
};

template<typename R>
StackedLSTM<R>::StackedLSTM(const int& input_size, const std::vector<int>& hidden_sizes) :
        AbstractStackedLSTM<R>(input_size, hidden_sizes) {
    cells = StackedCells<lstm_t>(input_size, hidden_sizes);
};

template<typename R>
StackedLSTM<R>::StackedLSTM(const StackedLSTM<R>& model, bool copy_w, bool copy_dw) :
        AbstractStackedLSTM<R>(model, copy_w, copy_dw) {
    cells = StackedCells<lstm_t>(model.cells, copy_w, copy_dw);
};

template<typename R>
StackedLSTM<R> StackedLSTM<R>::shallow_copy() const {
    return StackedLSTM<R>(*this, false, true);
}

template<typename R>
std::vector<Mat<R>> StackedLSTM<R>::parameters() const {
    vector<Mat<R>> parameters;
    for (auto& cell : cells) {
        auto cell_params = cell.parameters();
        parameters.insert(parameters.end(), cell_params.begin(), cell_params.end());
    }
    return parameters;
}

template<typename R>
typename StackedLSTM<R>::state_t StackedLSTM<R>::activate(
            state_t previous_state,
            Mat<R> input_vector,
            R drop_prob) const {
    return forward_LSTMs(input_vector, previous_state, cells, drop_prob);
};

template class StackedLSTM<float>;
template class StackedLSTM<double>;

template<typename R>
StackedShortcutLSTM<R>::StackedShortcutLSTM(const int& input_size, const std::vector<int>& hidden_sizes) :
    AbstractStackedLSTM<R>(input_size, hidden_sizes),
    base_cell(input_size, hidden_sizes[0]) {
    vector<int> sliced_hidden_sizes(hidden_sizes.begin()+1, hidden_sizes.end());
    // shortcut has size input size (base layer)
    // while input of shortcut cells is the hidden size:
    cells = StackedCells<shortcut_lstm_t>(hidden_sizes[0], input_size, sliced_hidden_sizes);
}

template<typename R>
StackedShortcutLSTM<R>::StackedShortcutLSTM(const StackedShortcutLSTM<R>& model, bool copy_w, bool copy_dw) :
    AbstractStackedLSTM<R>(model, copy_w, copy_dw),
    base_cell(model.base_cell),
    cells(StackedCells(model.cells, copy_w, copy_dw)) {};

template<typename R>
StackedShortcutLSTM<R> StackedShortcutLSTM<R>::shallow_copy() const {
    return StackedShortcutLSTM<R>(*this, false, true);
}

template<typename R>
std::vector<Mat<R>> StackedShortcutLSTM<R>::parameters() const {
    auto parameters = base_cell.parameters();
    for (auto& cell : cells) {
        auto cell_params = cell.parameters();
        parameters.insert(parameters.end(), cell_params.begin(), cell_params.end());
    }
    return parameters;
}

template<typename R>
typename StackedShortcutLSTM<R>::state_t StackedShortcutLSTM<R>::activate(
            state_t previous_state,
            Mat<R> input_vector,
            R drop_prob) const {
    // previous state is a pair of vectors of memory and hiddens.
    // hiddens is a vector of matrices of sizes decribed by hidden sizes
    #ifdef NDEBUG
        assert(input_vector->n == this->input_size);
        for (auto& memory_or_hidden : {get<0>(previous_state),
                                                   std::get<1>(previous_state)}) {
            assert(memory_or_hidden.n == this->hidden_sizes.size());
            for (int i=0; i < this->hidden_sizes.size(); ++i) {
                assert(memory_or_hidden[i]->n == this->hidden_sizes[i]);
            }
        }
    #endif
    return forward_LSTMs(input_vector, previous_state, base_cell, cells, drop_prob);
};

template class StackedShortcutLSTM<float>;
template class StackedShortcutLSTM<double>;
