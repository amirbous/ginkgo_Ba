// SPDX-FileCopyrightText: 2017 - 2024 The Ginkgo authors
//
// SPDX-License-Identifier: BSD-3-Clause

#include <ginkgo/core/config/config.hpp>


#include <typeinfo>


#include <gtest/gtest.h>


#include <ginkgo/core/base/executor.hpp>
#include <ginkgo/core/matrix/dense.hpp>
#include <ginkgo/core/solver/bicg.hpp>
#include <ginkgo/core/solver/cg.hpp>
#include <ginkgo/core/stop/combined.hpp>
#include <ginkgo/core/stop/iteration.hpp>
#include <ginkgo/core/stop/residual_norm.hpp>


#include "core/config/config_helper.hpp"
#include "core/test/utils.hpp"


namespace {


using namespace gko::config;


class Config : public ::testing::Test {
protected:
    using value_type = double;
    using Mtx = gko::matrix::Dense<value_type>;
    Config()
        : exec(gko::ReferenceExecutor::create()),
          mtx(gko::initialize<Mtx>(
              {{2, -1.0, 0.0}, {-1.0, 2, -1.0}, {0.0, -1.0, 2}}, exec)),
          stop_config({{"Type", pnode{"Iteration"}}, {"max_iters", pnode{1}}})
    {}

    std::shared_ptr<const gko::Executor> exec;
    std::shared_ptr<Mtx> mtx;
    pnode stop_config;
};


TEST_F(Config, GenerateObjectWithoutDefault)
{
    auto reg = registry();

    pnode p{{{"ValueType", pnode{"double"}}, {"criteria", this->stop_config}}};
    auto obj = parse<LinOpFactoryType::Cg>(p, reg).on(this->exec);

    ASSERT_NE(dynamic_cast<const gko::solver::Cg<double>::Factory*>(obj.get()),
              nullptr);
}


TEST_F(Config, GenerateObjectWithData)
{
    auto reg = registry();
    reg.emplace("precond", this->mtx);

    pnode p{{{"generated_preconditioner", pnode{"precond"}},
             {"criteria", this->stop_config}}};
    auto obj =
        parse<LinOpFactoryType::Cg>(p, reg, type_descriptor{"float", "void"})
            .on(this->exec);

    ASSERT_NE(dynamic_cast<gko::solver::Cg<float>::Factory*>(obj.get()),
              nullptr);
    ASSERT_NE(dynamic_cast<gko::solver::Cg<float>::Factory*>(obj.get())
                  ->get_parameters()
                  .generated_preconditioner,
              nullptr);
}


TEST_F(Config, GenerateObjectWithPreconditioner)
{
    auto reg = registry();
    auto precond_node =
        pnode{{{"Type", pnode{"solver::Cg"}}, {"criteria", this->stop_config}}};
    pnode p{{{"ValueType", pnode{"double"}},
             {"criteria", this->stop_config},
             {"preconditioner", precond_node}}};

    auto obj = parse<LinOpFactoryType::Cg>(p, reg).on(this->exec);

    ASSERT_NE(dynamic_cast<gko::solver::Cg<double>::Factory*>(obj.get()),
              nullptr);
    ASSERT_NE(dynamic_cast<gko::solver::Cg<double>::Factory*>(obj.get())
                  ->get_parameters()
                  .preconditioner,
              nullptr);
}


TEST_F(Config, GenerateObjectWithCustomBuild)
{
    configuration_map config_map;

    config_map["Custom"] = [](const pnode& config, const registry& context,
                              const type_descriptor& td_for_child) {
        return gko::solver::Bicg<double>::build().with_criteria(
            gko::stop::Iteration::build().with_max_iters(2u));
    };
    auto reg = registry(config_map);
    auto precond_node =
        pnode{std::map<std::string, pnode>{{"Type", pnode{"Custom"}}}};
    pnode p{{{"ValueType", pnode{"double"}},
             {"criteria", this->stop_config},
             {"preconditioner", precond_node}}};

    auto obj =
        parse<LinOpFactoryType::Cg>(p, reg, type_descriptor{"double", "void"})
            .on(this->exec);

    ASSERT_NE(dynamic_cast<gko::solver::Cg<double>::Factory*>(obj.get()),
              nullptr);
    ASSERT_NE(dynamic_cast<const gko::solver::Bicg<double>::Factory*>(
                  dynamic_cast<gko::solver::Cg<double>::Factory*>(obj.get())
                      ->get_parameters()
                      .preconditioner.get()),
              nullptr);
}


TEST(GetValue, IndexType)
{
    long long int value = 123;
    pnode config{value};

    ASSERT_EQ(get_value<int>(config), value);
    ASSERT_EQ(get_value<long>(config), value);
    ASSERT_EQ(get_value<unsigned>(config), value);
    ASSERT_EQ(get_value<long long int>(config), value);
    ASSERT_EQ(typeid(get_value<int>(config)), typeid(int));
    ASSERT_EQ(typeid(get_value<long>(config)), typeid(long));
    ASSERT_EQ(typeid(get_value<unsigned>(config)), typeid(unsigned));
    ASSERT_EQ(typeid(get_value<long long int>(config)), typeid(long long int));
}


TEST(GetValue, RealType)
{
    double value = 1.0;
    pnode config{value};

    ASSERT_EQ(get_value<float>(config), value);
    ASSERT_EQ(get_value<double>(config), value);
    ASSERT_EQ(typeid(get_value<float>(config)), typeid(float));
    ASSERT_EQ(typeid(get_value<double>(config)), typeid(double));
}


TEST(GetValue, ComplexType)
{
    double real = 1.0;
    double imag = -1.0;
    pnode config{real};
    pnode array_config{pnode::array_type{pnode{real}, pnode{imag}}};

    // Only one value
    ASSERT_EQ(get_value<std::complex<float>>(config),
              std::complex<float>(real));
    ASSERT_EQ(get_value<std::complex<double>>(config),
              std::complex<double>(real));
    ASSERT_EQ(typeid(get_value<std::complex<float>>(config)),
              typeid(std::complex<float>));
    ASSERT_EQ(typeid(get_value<std::complex<double>>(config)),
              typeid(std::complex<double>));
    // Two value [real, imag]
    ASSERT_EQ(get_value<std::complex<float>>(array_config),
              std::complex<float>(real, imag));
    ASSERT_EQ(get_value<std::complex<double>>(array_config),
              std::complex<double>(real, imag));
    ASSERT_EQ(typeid(get_value<std::complex<float>>(array_config)),
              typeid(std::complex<float>));
    ASSERT_EQ(typeid(get_value<std::complex<double>>(array_config)),
              typeid(std::complex<double>));
}


}  // namespace
