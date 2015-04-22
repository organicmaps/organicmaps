/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef BAYES_CLASSIFIER_HPP
#define BAYES_CLASSIFIER_HPP

#include <cmath>

#include <vector>

struct NormalDistribution
{
    NormalDistribution(const double mean, const double standard_deviation)
        : mean(mean), standard_deviation(standard_deviation)
    {
    }

    // FIXME implement log-probability version since its faster
    double density_function(const double val) const
    {
        const double x = val - mean;
        return 1.0 / (std::sqrt(2. * M_PI) * standard_deviation) *
               std::exp(-x * x / (standard_deviation * standard_deviation));
    }

    double mean;
    double standard_deviation;
};

struct LaplaceDistribution
{
    LaplaceDistribution(const double location, const double scale)
        : location(location), scale(scale)
    {
    }

    // FIXME implement log-probability version since its faster
    double density_function(const double val) const
    {
        const double x = std::abs(val - location);
        return 1.0 / (2. * scale) * std::exp(-x / scale);
    }

    double location;
    double scale;
};

template <typename PositiveDistributionT, typename NegativeDistributionT, typename ValueT>
class BayesClassifier
{
  public:
    enum class ClassLabel : unsigned
    {
        NEGATIVE,
        POSITIVE
    };
    using ClassificationT = std::pair<ClassLabel, double>;

    BayesClassifier(const PositiveDistributionT &positive_distribution,
                    const NegativeDistributionT &negative_distribution,
                    const double positive_apriori_probability)
        : positive_distribution(positive_distribution),
          negative_distribution(negative_distribution),
          positive_apriori_probability(positive_apriori_probability),
          negative_apriori_probability(1. - positive_apriori_probability)
    {
    }

    // Returns label and the probability of the label.
    ClassificationT classify(const ValueT &v) const
    {
        const double positive_postpriori =
            positive_apriori_probability * positive_distribution.density_function(v);
        const double negative_postpriori =
            negative_apriori_probability * negative_distribution.density_function(v);
        const double norm = positive_postpriori + negative_postpriori;

        if (positive_postpriori > negative_postpriori)
        {
            return std::make_pair(ClassLabel::POSITIVE, positive_postpriori / norm);
        }

        return std::make_pair(ClassLabel::NEGATIVE, negative_postpriori / norm);
    }

  private:
    PositiveDistributionT positive_distribution;
    NegativeDistributionT negative_distribution;
    double positive_apriori_probability;
    double negative_apriori_probability;
};

#endif // BAYES_CLASSIFIER_HPP
