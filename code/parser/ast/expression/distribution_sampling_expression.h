//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_DISTRIBUTION_SAMPLING_EXPRESSION_H
#define PLAJA_DISTRIBUTION_SAMPLING_EXPRESSION_H

#include "expression.h"
#include "../iterators/ast_iterator.h"

class DistributionSamplingExpression final: public Expression {
public:
    enum DistributionType { DISCRETE_UNIFORM, BERNOULLI, BINOMIAL, NEGATIVE_BINOMIAL, POISSON, GEOMETRIC, HYPER_GEOMETRIC, CONWAY_MAXWELL_POISSON, ZIPF, UNIFORM, NORMAL, LOG_NORMAL, BETA, CAUCHY, CHI, CHI_SQUARED, ERLANG, EXPONENTIAL, FISHER_SNEDECOR, GAMMA, INVERSE_GAMMA, LAPLACE, PARETO, RAYLEIGH, STABLE, STUDENT_T, WEIBULL, TRIAGULAR };
private:
    DistributionType distributionType;
    std::vector<std::unique_ptr<Expression>> args; // must *not* contain distribution sampling
public:
    explicit DistributionSamplingExpression(DistributionType distributionType);
    ~DistributionSamplingExpression() override;

    // static
    static const std::string& distribution_type_to_str(DistributionType distribution_type);
    static std::unique_ptr<DistributionType> str_to_distribution_type(const std::string& distribution_str);

    // construction:
    void reserve(std::size_t args_cap);
    [[maybe_unused]] void add_arg(std::unique_ptr<Expression>&& arg);

    // setter:
    inline void set_distribution_type(DistributionType distribution_type) { distributionType = distribution_type; }
    inline void set_element(std::unique_ptr<Expression>&& arg, std::size_t index) { PLAJA_ASSERT(index < args.size()) args[index] = std::move(arg); }

    // getter:
    [[nodiscard]] inline DistributionType get_distribution_type() const { return distributionType; }
    [[nodiscard]] inline std::size_t get_number_args() const { return args.size(); }
    inline Expression* get_arg(std::size_t index) { PLAJA_ASSERT(index < args.size()) return args[index].get(); }
    [[nodiscard]] inline const Expression* get_arg(std::size_t index) const { PLAJA_ASSERT(index < args.size()) return args[index].get(); }
    [[nodiscard]] inline AstConstIterator<Expression> argIterator() const { return AstConstIterator(args); }
    [[nodiscard]] inline AstIterator<Expression> argIterator() { return AstIterator(args); }

    // override:
    [[nodiscard]] bool is_constant() const override;
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
};

#endif //PLAJA_DISTRIBUTION_SAMPLING_EXPRESSION_H
