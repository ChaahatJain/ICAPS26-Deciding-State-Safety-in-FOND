//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_map>
#include "distribution_sampling_expression.h"
#include "../../../exception/not_implemented_exception.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/declaration_type.h"

DistributionSamplingExpression::DistributionSamplingExpression(DistributionType distribution_type):
    distributionType(distribution_type)
    , args() {}

DistributionSamplingExpression::~DistributionSamplingExpression() = default;

// static

namespace DISTRIBUTION_SAMPLING_EXPRESSION {
    const std::string distributionTypeToStr[] { "DiscreteUniform", "Bernoulli", "Binomial", "NegativeBinomial", "Poisson", "Geometric", "HyperGeometric", "ConwayMaxwellPoisson", "Zipf", "Uniform", "Normal", "LogNormal", "Beta", "Cauchy", "Chi", "ChiSquared", "Erlang", "Exponential", "FisherSnedecor", "Gamma", "InverseGamma", "Laplace", "Pareto", "Rayleigh",
                                                "Stable", "StudentT", "Weibull", "Triangular" }; // NOLINT(cert-err58-cpp)
    const std::unordered_map<std::string, DistributionSamplingExpression::DistributionType> strToDistributionType { { "DiscreteUniform",      DistributionSamplingExpression::DISCRETE_UNIFORM },
                                                                                                                    { "Bernoulli",            DistributionSamplingExpression::BERNOULLI },
                                                                                                                    { "Binomial",             DistributionSamplingExpression::BINOMIAL },
                                                                                                                    { "NegativeBinomial",     DistributionSamplingExpression::NEGATIVE_BINOMIAL },
                                                                                                                    { "Poisson",              DistributionSamplingExpression::POISSON },
                                                                                                                    { "Geometric",            DistributionSamplingExpression::GEOMETRIC },
                                                                                                                    { "HyperGeometric",       DistributionSamplingExpression::HYPER_GEOMETRIC },
                                                                                                                    { "ConwayMaxwellPoisson", DistributionSamplingExpression::CONWAY_MAXWELL_POISSON },
                                                                                                                    { "Zipf",                 DistributionSamplingExpression::ZIPF },
                                                                                                                    { "Uniform",              DistributionSamplingExpression::UNIFORM },
                                                                                                                    { "Normal",               DistributionSamplingExpression::NORMAL },
                                                                                                                    { "LogNormal",            DistributionSamplingExpression::LOG_NORMAL },
                                                                                                                    { "Beta",                 DistributionSamplingExpression::BETA },
                                                                                                                    { "Cauchy",               DistributionSamplingExpression::CAUCHY },
                                                                                                                    { "Chi",                  DistributionSamplingExpression::CHI },
                                                                                                                    { "ChiSquared",           DistributionSamplingExpression::CHI_SQUARED },
                                                                                                                    { "Erlang",               DistributionSamplingExpression::ERLANG },
                                                                                                                    { "Exponential",          DistributionSamplingExpression::EXPONENTIAL },
                                                                                                                    { "FisherSnedecor",       DistributionSamplingExpression::FISHER_SNEDECOR },
                                                                                                                    { "Gamma",                DistributionSamplingExpression::GAMMA },
                                                                                                                    { "InverseGamma",         DistributionSamplingExpression::INVERSE_GAMMA },
                                                                                                                    { "Laplace",              DistributionSamplingExpression::LAPLACE },
                                                                                                                    { "Pareto",               DistributionSamplingExpression::PARETO },
                                                                                                                    { "Rayleigh",             DistributionSamplingExpression::RAYLEIGH },
                                                                                                                    { "Stable",               DistributionSamplingExpression::STABLE },
                                                                                                                    { "StudentT",             DistributionSamplingExpression::STUDENT_T },
                                                                                                                    { "Weibull",              DistributionSamplingExpression::WEIBULL },
                                                                                                                    { "Triangular",           DistributionSamplingExpression::TRIAGULAR } }; // NOLINT(cert-err58-cpp)
}

const std::string& DistributionSamplingExpression::distribution_type_to_str(DistributionSamplingExpression::DistributionType distribution_type) { return DISTRIBUTION_SAMPLING_EXPRESSION::distributionTypeToStr[distribution_type]; }

std::unique_ptr<DistributionSamplingExpression::DistributionType> DistributionSamplingExpression::str_to_distribution_type(const std::string& distribution_str) {
    auto it = DISTRIBUTION_SAMPLING_EXPRESSION::strToDistributionType.find(distribution_str);
    if (it == DISTRIBUTION_SAMPLING_EXPRESSION::strToDistributionType.end()) { return nullptr; }
    else { return std::make_unique<DistributionSamplingExpression::DistributionType>(it->second); }
}

// construction:

void DistributionSamplingExpression::reserve(std::size_t args_cap) { args.reserve(args_cap); }

[[maybe_unused]] void DistributionSamplingExpression::add_arg(std::unique_ptr<Expression>&& arg) { args.push_back(std::move(arg)); }

// override:

bool DistributionSamplingExpression::is_constant() const { return false; }

const DeclarationType* DistributionSamplingExpression::determine_type() { throw NotImplementedException(__PRETTY_FUNCTION__); }

void DistributionSamplingExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void DistributionSamplingExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> DistributionSamplingExpression::deepCopy_Exp() const {
    std::unique_ptr<DistributionSamplingExpression> copy(new DistributionSamplingExpression(distributionType));
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    copy->reserve(args.size());
    for (const auto& arg: args) { copy->add_arg(arg->deepCopy_Exp()); }
    return copy;
}






