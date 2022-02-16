#pragma once

#include <memory>

#include "base/callback_iface.h"

namespace base {

namespace detail {
class BindAccessHelper;
}  // namespace detail

//
// OnceCallback<ReturnType(ArgumentTypes...)>
//

template <typename Signature>
class OnceCallback {
  static_assert(!sizeof(Signature), "Invalid template instantiation");
};

template <typename ReturnType, typename... ArgumentTypes>
class OnceCallback<ReturnType(ArgumentTypes...)> {
 public:
  OnceCallback() = default;
  OnceCallback(OnceCallback&& other) = default;
  OnceCallback& operator=(OnceCallback&&) = default;

  explicit operator bool() const { return !!impl_; }

  ReturnType Run(ArgumentTypes... arguments) && {
    OnceCallback callback = std::move(*this);
    return callback.impl_->Run(std::forward<ArgumentTypes>(arguments)...);
  }

 private:
  friend class detail::BindAccessHelper;

  explicit OnceCallback(
      std::unique_ptr<
          detail::OnceCallbackInterface<ReturnType, ArgumentTypes...>> impl)
      : impl_(std::move(impl)) {}

  std::unique_ptr<detail::OnceCallbackInterface<ReturnType, ArgumentTypes...>>
      impl_;
};

//
// RepeatingCallback<ReturnType(ArgumentTypes...)>
//

template <typename Signature>
class RepeatingCallback {
  static_assert(!sizeof(Signature), "Invalid template instantiation");
};

template <typename ReturnType, typename... ArgumentTypes>
class RepeatingCallback<ReturnType(ArgumentTypes...)> {
 public:
  RepeatingCallback() = default;
  RepeatingCallback(RepeatingCallback&&) = default;
  RepeatingCallback& operator=(RepeatingCallback&&) = default;

  RepeatingCallback(const RepeatingCallback& other)
      : impl_(CloneImpl(other.impl_)) {}

  RepeatingCallback& operator=(const RepeatingCallback& other) {
    impl_ = CloneImpl(other.impl_);
    return *this;
  }

  explicit operator OnceCallback<ReturnType(ArgumentTypes...)>() const {
    return OnceCallback<ReturnType(ArgumentTypes...)>{CloneImpl(impl_)};
  }

  explicit operator bool() const { return !!impl_; }

  ReturnType Run(ArgumentTypes... arguments) const& {
    return impl_->Run(std::forward<ArgumentTypes>(arguments)...);
  }

  ReturnType Run(ArgumentTypes... arguments) && {
    RepeatingCallback callback = std::move(*this);
    callback.Run(std::forward<ArgumentTypes>(arguments)...);
  }

 private:
  friend class detail::BindAccessHelper;

  explicit RepeatingCallback(
      std::unique_ptr<
          detail::RepeatingCallbackInterface<ReturnType, ArgumentTypes...>>
          impl)
      : impl_(std::move(impl)) {}

  using ImplPtr = std::unique_ptr<
      detail::RepeatingCallbackInterface<ReturnType, ArgumentTypes...>>;

  static ImplPtr CloneImpl(const ImplPtr& impl) {
    if (impl) {
      return impl->Clone();
    }
    return {};
  }

  ImplPtr impl_;
};

//
// Closure aliases.
//

using OnceClosure = OnceCallback<void()>;
using RepeatingClosure = RepeatingCallback<void()>;

//
// Callback traits
//

namespace traits {

template <typename CallbackType>
struct IsOnceCallbackType {
  static constexpr bool value = false;
};

template <typename CallbackSignatureType>
struct IsOnceCallbackType<::base::OnceCallback<CallbackSignatureType>> {
  static constexpr bool value = true;
};

template <typename CallbackType>
struct IsRepeatingCallbackType {
  static constexpr bool value = false;
};

template <typename CallbackSignatureType>
struct IsRepeatingCallbackType<
    ::base::RepeatingCallback<CallbackSignatureType>> {
  static constexpr bool value = true;
};

template <typename CallbackType>
inline constexpr bool IsOnceCallbackV = IsOnceCallbackType<CallbackType>::value;
template <typename CallbackType>
inline constexpr bool IsRepeatingCallbackV =
    IsRepeatingCallbackType<CallbackType>::value;
template <typename CallbackType>
inline constexpr bool IsCallbackV =
    IsOnceCallbackV<CallbackType> || IsRepeatingCallbackV<CallbackType>;

}  // namespace traits

}  // namespace base
