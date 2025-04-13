#pragma once

namespace succinct {

    namespace detail {
        template <typename BitVectorBuilder>
        void random_binary_tree_helper(BitVectorBuilder& builder, size_t size)
        {
            assert((size & 1) == 1); // binary trees can only have an odd number of nodes (internal + leaves)
            if (size == 1) {
                builder.push_back(0); // can only be a leaf
                return;
            }

            builder.push_back(1);
            size_t left_subtree_size = 2 * (size_t(rand()) % (size - 1) / 2) + 1;
            assert(left_subtree_size >= 1);
            size_t right_subtree_size = size - 1 - left_subtree_size;
            assert(right_subtree_size >= 1);
            assert(left_subtree_size + right_subtree_size + 1 == size);

            random_binary_tree_helper(builder, left_subtree_size);
            random_binary_tree_helper(builder, right_subtree_size);
        }
    }


    template <typename BitVectorBuilder>
    void random_binary_tree(BitVectorBuilder& builder, size_t size)
    {
        assert((size & 1) == 0 && size >= 2);

        builder.push_back(1); // fake root
        detail::random_binary_tree_helper(builder, size - 1);
    }

    template <typename BitVectorBuilder>
    void random_bp(BitVectorBuilder& builder, size_t size_est)
    {
        int excess = 0;
        for (size_t i = 0; i < size_est; ++i) {
            bool val = rand() > (RAND_MAX / 2);
            if (excess <= 1 && !val) {
                val = 1;
            }
            excess += (val ? 1 : -1);
            builder.push_back(val);
        }

        for (size_t i = 0; i < size_t(excess); ++i) {
            builder.push_back(0); // close all parentheses
        }
    }

    template <typename BitVectorBuilder>
    void bp_path(BitVectorBuilder& builder, size_t size)
    {
        assert((size & 1) == 0);
        for (size_t i = 0; i < size / 2; ++i) {
            builder.push_back(1);
        }
        for (size_t i = 0; i < size / 2; ++i) {
            builder.push_back(0);
        }
    }


}
