// SPDX-FileCopyrightText: 2017-2023 The Ginkgo authors
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef GINKGO_SPACES_HPP
#define GINKGO_SPACES_HPP

#include <ginkgo/config.hpp>


#if GINKGO_EXTENSION_KOKKOS


#include <Kokkos_Core.hpp>


#include <ginkgo/core/base/executor.hpp>


namespace gko {
namespace ext {
namespace kokkos {
namespace detail {


/**
 * Helper to check if an executor type can access the memory of an memory space
 *
 * @tparam MemorySpace  Type fulfilling the Kokkos MemorySpace concept.
 * @tparam ExecType  One of the Ginkgo executor types.
 */
template <typename MemorySpace, typename ExecType>
struct compatible_space
    : std::integral_constant<bool, Kokkos::has_shared_space ||
                                       Kokkos::has_shared_host_pinned_space> {};

template <>
struct compatible_space<Kokkos::HostSpace, gko::ReferenceExecutor>
    : std::true_type {};

template <typename MemorySpace>
struct compatible_space<MemorySpace, gko::ReferenceExecutor> {
    // need manual implementation of std::integral_constant because,
    // while compiling for cuda, somehow bool is replaced by __nv_bool
    static constexpr bool value =
        Kokkos::SpaceAccessibility<Kokkos::HostSpace, MemorySpace>::accessible;
};
#ifdef KOKKOS_ENABLE_OPENMP
template <typename MemorySpace>
struct compatible_space<MemorySpace, gko::OmpExecutor>
    : compatible_space<MemorySpace, gko::ReferenceExecutor> {};
#endif

#ifdef KOKKOS_ENABLE_CUDA
template <typename MemorySpace>
struct compatible_space<MemorySpace, gko::CudaExecutor> {
    static constexpr bool value =
        Kokkos::SpaceAccessibility<Kokkos::Cuda, MemorySpace>::accessible;
};
#endif

#ifdef KOKKOS_ENABLE_HIP
template <typename MemorySpace>
struct compatible_space<MemorySpace, gko::HipExecutor> {
    static constexpr bool value =
        Kokkos::SpaceAccessibility<Kokkos::HIP, MemorySpace>::accessible;
};
#endif

#ifdef KOKKOS_ENABLE_SYCL
template <typename MemorySpace>
struct compatible_space<MemorySpace, gko::DpcppExecutor> {
    static constexpr bool value =
        Kokkos::SpaceAccessibility<Kokkos::Experimental::SYCL,
                                   MemorySpace>::accessible;
};
#endif


/**
 * Checks if the memory space is accessible by the executor
 *
 * @tparam MemorySpace  A Kokkos memory space type
 * @tparam ExecType  A Ginkgo executor type
 *
 * @return  true if the memory space is accessible by the executor
 */
template <typename MemorySpace, typename ExecType>
inline bool check_compatibility(std::shared_ptr<const ExecType>)
{
    return detail::compatible_space<MemorySpace, ExecType>::value;
}


/**
 * Checks if the memory space is accessible by the executor
 *
 * @tparam MemorySpace  A Kokkos memory space
 * @param exec  The executor which should access the memory space
 *
 * @return  true if the memory space is accessible by the executor
 */

template <typename MemorySpace>
inline bool check_compatibility(std::shared_ptr<const Executor> exec)
{
    if (auto p = std::dynamic_pointer_cast<const ReferenceExecutor>(exec)) {
        return check_compatibility<MemorySpace>(p);
    }
    if (auto p = std::dynamic_pointer_cast<const OmpExecutor>(exec)) {
        return check_compatibility<MemorySpace>(p);
    }
    if (auto p = std::dynamic_pointer_cast<const CudaExecutor>(exec)) {
        return check_compatibility<MemorySpace>(p);
    }
    if (auto p = std::dynamic_pointer_cast<const HipExecutor>(exec)) {
        return check_compatibility<MemorySpace>(p);
    }
    if (auto p = std::dynamic_pointer_cast<const DpcppExecutor>(exec)) {
        return check_compatibility<MemorySpace>(p);
    }
    GKO_NOT_IMPLEMENTED;
}


/**
 * Throws if the memory space is ~not~ accessible by the executor associated
 * with the passed in Ginkgo object.
 *
 * @tparam MemorySpace  A Kokkos memory space type..
 * @tparam T  A Ginkgo type that has the member function `get_executor`.
 *
 * @param obj  Object which executor is used to check the access to the memory
 * space.
 * @param space  The Kokkos memory space to compare agains.
 */
template <typename MemorySpace, typename T>
inline void assert_compatibility(T&& obj, MemorySpace space)
{
    GKO_THROW_IF_INVALID(check_compatibility<MemorySpace>(obj.get_executor()),
                         "Executor type and memory space are incompatible");
}


}  // namespace detail


/**
 * Creates an Executor matching the Kokkos::DefaultHostExecutionSpace.
 *
 * If no kokkos host execution space is enabled, this throws an exception.
 *
 * @return  An executor of type either ReferenceExecutor or OmpExecutor.
 */
inline std::shared_ptr<Executor> create_default_host_executor()
{
    static std::mutex mutex{};
    std::lock_guard<std::mutex> guard(mutex);
#ifdef KOKKOS_ENABLE_SERIAL
    if constexpr (std::is_same_v<Kokkos::DefaultHostExecutionSpace,
                                 Kokkos::Serial>) {
        static auto exec = ReferenceExecutor::create();
        return exec;
    }
#endif
#ifdef KOKKOS_ENABLE_OPENMP
    if constexpr (std::is_same_v<Kokkos::DefaultHostExecutionSpace,
                                 Kokkos::OpenMP>) {
        static auto exec = OmpExecutor::create();
        return exec;
    }
#endif
    GKO_NOT_IMPLEMENTED;
}


/**
 * Creates an Executor for a specific Kokkos ExecutionSpace.
 *
 * This function supports the following Kokkos ExecutionSpaces:
 * - Serial
 * - OpenMP
 * - Cuda
 * - HIP
 * - Experimental::SYCL
 * If none of these spaces are enabled, then this function throws an exception.
 * For Cuda, HIP, SYCL, the device-id used by Kokkos is passed to the Executor
 * constructor.
 *
 * @tparam ExecSpace  A supported Kokkos ExecutionSpace.
 *
 * @return  An executor matching the type of the ExecSpace.
 */
template <typename ExecSpace,
          typename MemorySpace = typename ExecSpace::memory_space>
inline std::shared_ptr<Executor> create_executor(ExecSpace ex, MemorySpace = {})
{
    static_assert(
        Kokkos::SpaceAccessibility<ExecSpace, MemorySpace>::accessible);
    static std::mutex mutex{};
    std::lock_guard<std::mutex> guard(mutex);
#ifdef KOKKOS_ENABLE_SERIAL
    if constexpr (std::is_same_v<ExecSpace, Kokkos::Serial>) {
        return ReferenceExecutor::create();
    }
#endif
#ifdef KOKKOS_ENABLE_OPENMP
    if constexpr (std::is_same_v<ExecSpace, Kokkos::OpenMP>) {
        return OmpExecutor::create();
    }
#endif
#ifdef KOKKOS_ENABLE_CUDA
    if constexpr (std::is_same_v<ExecSpace, Kokkos::Cuda>) {
        if constexpr (std::is_same_v<MemorySpace, Kokkos::CudaSpace>) {
            return CudaExecutor::create(
                Kokkos::device_id(), create_default_host_executor(),
                std::make_shared<gko::CudaAllocator>(), ex.cuda_stream());
        }
        if constexpr (std::is_same_v<MemorySpace, Kokkos::CudaUVMSpace>) {
            return CudaExecutor::create(
                Kokkos::device_id(), create_default_host_executor(),
                std::make_shared<gko::CudaUnifiedAllocator>(
                    Kokkos::device_id()),
                ex.cuda_stream());
        }
        if constexpr (std::is_same_v<MemorySpace,
                                     Kokkos::CudaHostPinnedSpace>) {
            return CudaExecutor::create(
                Kokkos::device_id(), create_default_host_executor(),
                std::make_shared<gko::CudaHostAllocator>(Kokkos::device_id()),
                ex.cuda_stream());
        }
    }
#endif
#ifdef KOKKOS_ENABLE_HIP
    if constexpr (std::is_same_v<ExecSpace, Kokkos::HIP>) {
        if constexpr (std::is_same_v<MemorySpace, Kokkos::HIPSpace>) {
            return HipExecutor::create(
                Kokkos::device_id(), create_default_host_executor(),
                std::make_shared<gko::HipAllocator>(), ex.hip_stream());
        }
        if constexpr (std::is_same_v<MemorySpace, Kokkos::HIPManagedSpace>) {
            return HipExecutor::create(
                Kokkos::device_id(), create_default_host_executor(),
                std::make_shared<gko::HipUnifiedAllocator>(Kokkos::device_id()),
                ex.hip_stream());
        }
        if constexpr (std::is_same_v<MemorySpace, Kokkos::HIPHostPinnedSpace>) {
            return HipExecutor::create(
                Kokkos::device_id(), create_default_host_executor(),
                std::make_shared<gko::HipHostAllocator>(Kokkos::device_id()),
                ex.hip_stream());
        }
    }
#endif
#ifdef KOKKOS_ENABLE_SYCL
    if constexpr (std::is_same_v<ExecSpace, Kokkos::Experimental::SYCL>) {
        static_assert(
            std::is_same_v<MemorySpace, Kokkos::Experimental::SYCLSpace>,
            "Ginkgo doesn't support shared memory space allocation for SYCL");
        return DpcppExecutor::create(Kokkos::device_id(),
                                     create_default_host_executor());
    }
#endif
    GKO_NOT_IMPLEMENTED;
}


/**
 * Creates an Executor matching the Kokkos::DefaultExecutionSpace.
 *
 * @return  An executor matching the type of Kokkos::DefaultExecutionSpace.
 */
inline std::shared_ptr<Executor> create_default_executor(
    Kokkos::DefaultExecutionSpace ex = {})
{
    return create_executor(std::move(ex));
}


}  // namespace kokkos
}  // namespace ext
}  // namespace gko


#endif  // GINKGO_EXTENSION_KOKKOS
#endif  // GINKGO_SPACES_HPP
