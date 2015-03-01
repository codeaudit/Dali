#ifndef CORE_MODEL_RECURRENT_H
#define CORE_MODEL_RECURRENT_H

#include <string>
#include <vector>

#include "core/model/Model.h"
#include "core/model/Layer.h"


namespace model {
    template <typename REAL_t>
    class RNN: public RecurrentModel<REAL_t> {
        int input_size;
        int output_size;
        int memory_size;

        Layer<REAL_t> input_map;
        Layer<REAL_t> output_map;
        Layer<REAL_t> memory_map;
        SHARED_MAT first_memory;

        SHARED_MAT prev_memory;

        public:
            RNN(int input_size,
                int output_size,
                int memory_size,
                double bound=0.2);

            void reset();

            // output is in range 0, 1
            SHARED_MAT activate(GRAPH& G, SHARED_MAT input);

            std::vector<SHARED_MAT> parameters() const;
    };
}

#endif